// os.cpp

#include "stdafx.h"
#include <time.h>
#include "math.hpp"
#include "ref.hpp"
#include "sq.hpp"
#include "stream.hpp"
#include "string.hpp"
#include "win32.hpp"
#include "resource.h"

//=============================================================================

static SQInteger os_copy(sq::VM v);
static SQInteger os_bury(sq::VM v);
static SQInteger os_remove(sq::VM v);
static SQInteger os_rename(sq::VM v);

//=============================================================================

static void append_path(StringBuffer& buffer, PCWSTR src, WCHAR sep = '\0')
{
	WCHAR	path[MAX_PATH];
	KnownFolderResolve(path, src);

	size_t length = wcslen(path);
	WCHAR	from	= '/';
	WCHAR	to		= '\\';
	bool quote = (sep == ' ' && wcschr(path, ' '));

	buffer.reserve(buffer.size() + length + 4);

	if (quote)
		buffer.append('"');

	size_t sz = buffer.size();
	buffer.resize(sz + length);
	PWSTR buf = buffer.data();
	for (size_t i = 0; i < length; ++i)
	{
		if (path[i] == from)
			buf[sz+i] = to;
		else
			buf[sz+i] = path[i];
	}

	if (quote)
		buffer.append('"');

	buffer.append(sep);
}

static void append_unpacked(sq::VM v, StringBuffer& s, int index)
{
	foreach (v, index)
	{
		if (sq_gettype(v, -1) == OT_STRING)
			append_path(s, v.get<PCWSTR>(-1));
		else
			append_unpacked(v, s, -1);	// 再起呼び出し
	}
}

static SQInteger fileop(sq::VM v, UINT what, FILEOP_FLAGS flags)
{
	bool		binop = (what != FO_DELETE);
	SQInteger	n = v.gettop();

	// スラッシュをバックスラッシュに変換しないと SHFileOperation が誤動作するため、
	// StringBuffer にコピーして書き換える。
	StringBuffer	src;
	StringBuffer	dst;

	SQInteger ln = (binop ? n - 1 : n);
	for (int i = 2; i <= ln; ++i)
	{
		if (sq_gettype(v, -1) == OT_STRING)
			append_path(src, v.get<PCWSTR>(i));
		else
			append_unpacked(v, src, i);	// 再起呼び出し
	}
	src.terminate();

	if (binop)
	{
		append_path(dst, v.get<PCWSTR>(n));
		dst.terminate();	// 念のため
	}

	FileOperate(what, flags, src, dst);
	return 0;
}

//=============================================================================

// os.copy( { src, ... } , dst )
// os.copy( src1, src2, ... , dst )
static SQInteger os_copy(sq::VM v)
{
	return fileop(v, FO_COPY, FOF_ALLOWUNDO);
}

static bool os_paste(PCWSTR dst)
{
	return SUCCEEDED(FilePaste(dst));
}

static SQInteger os_remove(sq::VM v)
{
	// if (filepath is network path) then use os_bury instead.
	return fileop(v, FO_DELETE, FOF_ALLOWUNDO);
}

static SQInteger os_bury(sq::VM v)
{
	return fileop(v, FO_DELETE, 0);
}

static SQInteger os_rename(sq::VM v)
{
	return fileop(v, FO_MOVE, FOF_ALLOWUNDO);
}

static string os_time(PCWSTR format)
{
	StringBuffer fmt;
	if (str::empty(format))
		format = L"%Y-%m-%d %H:%M:%S";
	else
	{
		for (PCWSTR s = format; *s; ++s)
		{
			if (*s != '%')
			{
				fmt.append(*s);
				continue;
			}

			// サポートしていない %<文字> を渡すと CRT が segfault するため、あらかじめフィルタリングする。
			++s;
			if (*s == '\0')
				break;
			else if (*s == 'F')
				fmt.append(L"%Y-%m-%d");
			else if (*s == 'T')
				fmt.append(L"%H:%M:%S");
			else if (strchr("aAbBcdHIjmMpSUwWxXyYzZ%", *s))
			{
				fmt.append(L'%');
				fmt.append(*s);
			}
			else
			{
				fmt.append(L'%');
				fmt.append(L'%');
				fmt.append(*s);
			}
		}
		fmt.terminate();
		format = fmt.data();
	}

	time_t		t = time(NULL);
	struct tm	tm;
	localtime_s(&tm, &t);
	WCHAR	buffer[1024];
	wcsftime(buffer, lengthof(buffer), format, &tm);
	return str::dup(buffer);
}

static ref<IShellItemArray> os_dir(PCWSTR path)
{
	if (ILPtr item = ILCreate(path))
	{
		ref<IShellItemArray>	items;
		ref<IShellFolder>		folder;
		ref<IEnumIDList>		enumerator;
		if (SUCCEEDED(ILFolder(&folder, item)) &&
			SUCCEEDED(folder->EnumObjects(GetWindow(), SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &enumerator)) &&
			SUCCEEDED(PathsCreate(&items, item, folder, enumerator)))
			return items;
	}
	return null;
}

static ref<IShellItem> os_path(PCWSTR path)
{
	HRESULT hr;
	ref<IShellItem> item;
	if FAILED(hr = PathCreate(&item, path))
		throw sq::Error(hr, L"os.path(\"%s\") failed", path);
	return item;
}

/* File size in recycle bin in kB */
static int trash_size()
{
	SHQUERYRBINFO info = { sizeof(SHQUERYRBINFO) };
	SHQueryRecycleBin(null, &info);
	if (info.i64NumItems < 1)
		return 0;
	// int32に収まるようkBで返す。0 にならないよう調整する。
	return math::max(1, (int)((info.i64Size + 1023) / 1024));
}

static void trash_purge()
{
	SHEmptyRecycleBin(GetWindow(), null, 0);
}

static void os_settings()
{
	// プロトタイプがいまいちよく分からないので、ShellExecute経由で呼ぶ
	ShellExecute(GetWindow(), null, L"rundll32.exe", L"shell32.dll,Options_RunDLL 0", null, SW_SHOW);

	// XPだと、このアプリケーションよりも背面に表示してしまうので、前面化する。
	// TODO: 多国語対応のため、ダイアログ名を Windows リソースから取得すべき。
	if (WINDOWS_VERSION < WINDOWS_VISTA)
	{
		HWND hFolderOptions = null;
		for (int i = 0; i < 10; ++i)
		{
			PumpMessage();
			hFolderOptions = ::FindWindow(L"#32770", L"フォルダ オプション");
			if (hFolderOptions)
				break;
			Sleep(100);
		}
		if (hFolderOptions)
		{
			::BringWindowToTop(hFolderOptions);
			::SetActiveWindow(hFolderOptions);
		}
	}
}

//=============================================================================

template < typename T, typename Base >
interface __declspec(novtable) Wrapper : public Base
{
	static HSQOBJECT vtbl;

	static void push(HSQUIRRELVM v, IUnknown* value)
	{
		sq::push(v, value, &vtbl, &T::def);
	}
};

template < typename T, typename Base >
HSQOBJECT Wrapper<T, Base>::vtbl = { OT_NULL, NULL };

//=============================================================================

namespace
{
	// same as IShellItem
	MIDL_INTERFACE("43826d1e-e718-42ee-bc55-a1e261c37bfe")
	Path : public Wrapper<Path, IShellItem>
	{
	public:
		CoStr					_tostring();
		string					get_id();
		CoStr					get_name();
		CoStr					get_path();
		int						get_icon();
		int						get_attr();
		SQInteger				get_children(sq::VM v);
		ref<IShellItem>			get_linked();
		ref<IShellItem>			get_parent();
		bool					browsable(bool nonDefault);
		void					execute(PCWSTR verb, PCWSTR args);
		ref<IShellItem>			parse(PCWSTR name);
		bool					popup();
		void					symlink(PCWSTR path);

		static SQInteger def(sq::VM v)
		{
#define CLASS	Path
			def_method(_tostring);
			def_method(get_attr);
			def_method(get_children);
			def_method(get_id);
			def_method(get_linked);
			def_method(get_name);
			def_method(get_parent);
			def_method(get_path);
			def_method(get_icon);
			def_method(browsable);
			def_method(execute);
			def_method(parse);
			def_method(popup);
			def_method(symlink);
#undef CLASS
			return 1;
		}

	private:
		bool	has_attributes(SFGAOF sfgaof) throw();
		CoStr	get_displayname(SIGDN sigdn) throw();
	};
}

bool Path::has_attributes(SFGAOF sfgaof) throw()
{
	SFGAOF flags = 0;
	if FAILED(GetAttributes(sfgaof, &flags))
		return false;
	return (flags & sfgaof) != 0;
}

CoStr Path::get_displayname(SIGDN sigdn) throw()
{
	CoStr name;
	GetDisplayName(sigdn, &name);
	return name;
}

CoStr Path::_tostring()
{
	return get_name();
}

string Path::get_id()
{
	return KnownFolderEncode(get_displayname(SIGDN_DESKTOPABSOLUTEPARSING));
}

CoStr Path::get_name()
{
	CoStr value = get_displayname(SIGDN_PARENTRELATIVE);
	if (!value)
	{
		_S s(STR_DESKTOP);
		size_t sz = (wcslen(s) + 1) * sizeof(WCHAR);
		value.realloc(sz);
		memcpy(value, s, sz);
	}
	return value;
}

CoStr Path::get_path()
{
	return get_displayname(SIGDN_FILESYSPATH);
}

int Path::get_icon()
{
	return MAKELONG(ILIconIndex(ILCreate(this)), 1);
}

const SFGAOF SFGAO_ALL = SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_ENCRYPTED | SFGAO_ISSLOW | SFGAO_GHOSTED | SFGAO_LINK | SFGAO_SHARE | SFGAO_READONLY | SFGAO_HIDDEN | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_REMOVABLE | SFGAO_COMPRESSED | SFGAO_BROWSABLE;

int Path::get_attr()
{
	SFGAOF flags = 0;
	GetAttributes(SFGAO_ALL, &flags);
	return (int)flags;
}

ref<IShellItem> Path::get_parent()
{
	ref<IShellItem>	item;
	GetParent(&item);
	return item;
}

// get_children(filter = null)
SQInteger Path::get_children(sq::VM v)
{
	// BHID_EnumItems では隠しファイルとフォルダが取得できる。
	// BHID_StorageEnum ではシンボリックリンクも取得できてしまう？
	ref<IEnumShellItems> e;
	if (FAILED(BindToHandler(null, BHID_EnumItems, IID_PPV_ARGS(&e))) &&
		FAILED(BindToHandler(null, BHID_StorageEnum, IID_PPV_ARGS(&e))))
		return 0;

	SQInteger top = v.gettop();

	v.newarray();
	ref<IShellItem> item;
	while (item = null, e->Next(1, &item, NULL) == S_OK)
	{
		if (top > 2 && sq_gettype(v, 2) != OT_NULL)
		{
			// this.filter(item);
			sq_push(v, 2); 
			sq_pushroottable(v);
			v.push(item);
			sq::call(v, 2, SQTrue);
			bool includes = v.get<bool>(-1);
			v.settop(top + 1);
			if (!includes)
				continue;
		}

		v.push(item);
		v.append(-2);
	}
	return 1;
}

ref<IShellItem> Path::get_linked()
{
//	BHID_LinkTargetItem
	if (CoStr path = get_path())
	{
		ILPtr	linked;
		if SUCCEEDED(LinkResolve(path, &linked))
		{
			ref<IShellItem>	item;
			PathCreate(&item, linked);
			return item;
		}
	}

	// .lnk でない場合は自身を返す
	return ref<IShellItem>(this);
}

bool Path::browsable(bool nonDefault)
{
	switch (PathBrowsable(this))
	{
	case S_OK:
		return true;
	case S_FALSE:
		return nonDefault;
	default:
		return false;
	}
}

void Path::execute(PCWSTR verb, PCWSTR args)
{
	ILExecute(ILCreate(this), verb, args);
}

ref<IShellItem> Path::parse(PCWSTR name)
{
	if (wcscmp(name, L"..") == 0)
		return get_parent();
	ref<IShellItem>	item;
	if ((PathIsRelative(name) &&
		 SUCCEEDED(PathCreate(&item, this, name))) ||
		 SUCCEEDED(PathCreate(&item, name)))
		return item;
	throw sq::Error(L"Cannot parse a name: %s", name);
}

bool Path::popup()
{
	HRESULT				hr;
	ref<IContextMenu>	menu;

	if (FAILED(BindToHandler(null, BHID_SFUIObject, IID_PPV_ARGS(&menu))))
		return true;

	switch (hr = MenuPopup(GetCursorPos(), menu, _S(STR_GO)))
	{
	case S_DEFAULT:
		ResetKeyState(0);
		return false;	// 呼び出し側で処理を行う必要がある。
	case S_RENAME:
		Alert(_S(STR_E_SUPPORT), null, null, MB_ICONERROR);
		return true;
	default:
		return true;
	}
}

// Vista: BOOL CreateSymbolicLink(srcpath, dstpath, SYMLINK_FLAG_DIRECTORY or 0);
void Path::symlink(PCWSTR path)
{
	WCHAR dst[MAX_PATH];
	KnownFolderResolve(dst, path);

	// 拡張子は強制的に .lnk
	if (!has_attributes(SFGAO_FOLDER))
	{
		PathRemoveExtension(dst);
		PathAddExtension(dst, L".lnk");
	}
	else if (_wcsicmp(PathFindExtension(dst), L".lnk") != 0)
	{
		wcscat_s(dst, L".lnk");
	}

	if FAILED(LinkCreate(ILCreate(this), dst))
		throw sq::Error(_S(STR_E_SHORTCUT), dst);
}

void sq::push(HSQUIRRELVM v, IShellItem* value) throw()
{
	Path::push(v, value);
}

//=============================================================================

void sq::push(HSQUIRRELVM v, IShellItemArray* value) throw()
{
	if (value)
	{
		DWORD count = 0;
		value->GetCount(&count);
		sq_newarray(v, 0);
		for (DWORD i = 0; i < count; ++i)
		{
			ref<IShellItem> item;
			if SUCCEEDED(value->GetItemAt(i, &item))
			{
				sq::push(v, item);
				sq_arrayappend(v, -2);
			}
		}
	}
	else
		sq_pushnull(v);
}

//=============================================================================

SQInteger def_os(sq::VM v)
{
	v.push(L"os");
	v.newtable();
	v.def(L"bury"	, os_bury);
	v.def(L"copy"	, os_copy);
	v.def(L"dir"	, os_dir);
	v.def(L"paste"	, os_paste);
	v.def(L"path"	, os_path);
	v.def(L"remove"	, os_remove);
	v.def(L"rename"	, os_rename);
	v.def(L"drives"	, GetLogicalDrives);
	v.def(L"time"	, os_time);
	v.def(L"clipboard_get_text"	, ClipboardGetText);
	v.def(L"clipboard_set_text"	, ClipboardSetText);
	v.def(L"trash_size"			, trash_size);
	v.def(L"trash_purge"		, trash_purge);
	v.def(L"settings"			, os_settings);
	v.newslot(-3);

	return 1;
}
