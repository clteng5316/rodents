// itemid.cpp

#include "stdafx.h"
#include "math.hpp"
#include "stream.hpp"
#include "string.hpp"
#include "ref.hpp"
#include "win32.hpp"
#include "resource.h"

//==========================================================================================================================

static ILPtr ILCreateFromShmem(HANDLE mem, DWORD pid) throw()
{
	ILPtr pidl;
	if (ITEMIDLIST* pidlShared = (ITEMIDLIST*)SHLockShared(mem, pid))
	{
		if (!IsBadReadPtr(pidlShared, 1))
			pidl.attach(ILClone(pidlShared));
		SHUnlockShared(pidlShared);
		SHFreeShared(mem, pid);
	}
	return pidl;
}

ILPtr ILCreate(PCWSTR path_) throw()
{
	ILPtr		pidl;

	if (str::empty(path_))
		return null;

	WCHAR path[MAX_PATH];
	PathCanonicalize(path, path_);

	// SHILCreateFromPath は / を扱えないため、自前で変換する。
	for (size_t i = 0; path[i]; ++i)
		if (path[i] == L'/')
			path[i] = L'\\';

	// 独自のパス解釈を行う。
	ref<IKnownFolder>	known;
	PCWSTR				leaf;

	if SUCCEEDED(KnownFolderCreate(&known, path, &leaf))
	{
		if (leaf)
		{
			ref<IShellItem> parent;
			ref<IShellItem> item;
			if (SUCCEEDED(known->GetShellItem(0, IID_PPV_ARGS(&parent))) &&
				SUCCEEDED(PathCreate(&item, parent, leaf)))
				return ILCreate(item);
		}
		else if SUCCEEDED(known->GetIDList(0, &pidl))
				return pidl;
	}

	// ITEMIDLIST を作成する。
	if SUCCEEDED(::SHILCreateFromPath(path, &pidl, null))
        return pidl;

	// ":mem:pid" 形式ならば、共有メモリ経由での受け渡し。
	size_t ln = wcslen(path);
	if (ln < 4 || path[0] != L':')
		return pidl;
	PCWSTR nextColon = wcschr(path+1, L':');
	if (!nextColon)
		return pidl;
	HANDLE mem = (HANDLE)(INT_PTR)_wtoi(path+1);
	DWORD pid = _wtoi(nextColon+1);
	return ILCreateFromShmem(mem, pid);
}

ILPtr ILCreate(IShellFolder* folder, PCWSTR relname) throw()
{
	ILPtr leaf;
	if (folder && relname)
	{
		ULONG eaten = 0;
		folder->ParseDisplayName(GetWindow(), null, const_cast<PWSTR>(relname), &eaten, &leaf, null);
	}
	return leaf;
}

ILPtr ILCreate(const CIDA* items, size_t index) throw()
{
	ILPtr	pidl;
	if (const ITEMIDLIST* leaf = CIDAChild(items, index))
		pidl.attach(ILCombine(CIDAParent(items), leaf));
	return pidl;
}

ILPtr ILCreate(IPersistIDList* persist) throw()
{
	ILPtr	pidl;
	if (persist)
		persist->GetIDList(&pidl);
	return pidl;
}

ILPtr ILCreate(IUnknown* obj) throw()
{
	ILPtr	pidl;
	if (obj)
	{
		ref<IPersistIDList> persist;
		if SUCCEEDED(obj->QueryInterface(&persist))
			persist->GetIDList(&pidl);
	}
	return pidl;
}

ILPtr ILCloneParent(const ITEMIDLIST* item) throw()
{
	ILPtr pidl;
	if (item && !ILIsRoot(item))
	{
		ITEMIDLIST* leaf = ILFindLastID(item);
		size_t size = ((BYTE*)leaf - (BYTE*)item);
		pidl.realloc(size + sizeof(USHORT));
		memcpy(pidl, item, size);
		*(USHORT*)((BYTE*)pidl.ptr + size) = 0;
	}
	return pidl;
}

HRESULT ILStat(const ITEMIDLIST* item, WIN32_FIND_DATA* stat)
{
	HRESULT hr;
	WCHAR path[MAX_PATH];
	if SUCCEEDED(ILPath(item, path))
	{
		if (HANDLE handle = FindFirstFile(path, stat))
		{
			FindClose(handle);
			return S_OK;
		}
	}

	ref<IShellFolder> folder;
	const ITEMIDLIST* leaf;
	if FAILED(hr = ILParentFolder(&folder, item, &leaf))
		return hr;
	return SHGetDataFromIDList(folder, leaf, SHGDFIL_FINDDATA, stat, sizeof(WIN32_FIND_DATA));
}

HRESULT ILParentFolder(IShellFolder** pp, const ITEMIDLIST* item, const ITEMIDLIST** leaf) throw()
{
	return SHBindToParent(item, __uuidof(IShellFolder), (void**)pp, leaf);
}

HRESULT ILFolder(IShellFolder** pp, const ITEMIDLIST* item) throw()
{
	HRESULT hr;

	if (!item)
		return E_POINTER;
	if (ILIsRoot(item))
		return SHGetDesktopFolder(pp);

	ref<IShellFolder> parent;
	const ITEMIDLIST* leaf;
	if FAILED(hr = ILParentFolder(&parent, item, &leaf))
		return hr;

	return parent->BindToObject(leaf, null, __uuidof(IShellFolder), (void**)pp);
}

HRESULT ILContextMenu(IContextMenu** pp, const ITEMIDLIST* item) throw()
{
	HRESULT hr;
	ref<IShellFolder> parent;
	const ITEMIDLIST* leaf = null;
	if FAILED(hr = ILParentFolder(&parent, item, &leaf))
		return hr;
	return parent->GetUIObjectOf(GetWindow(), 1, &leaf, __uuidof(IContextMenu), null, (void**)pp);
}

HRESULT ILDisplayName(IShellFolder* folder, const ITEMIDLIST* leaf, DWORD shgdn, WCHAR name[MAX_PATH])
{
	STRRET strret = { 0 };
	HRESULT hr = folder->GetDisplayNameOf(leaf, shgdn, &strret);
	if FAILED(hr)
		return hr;
	return StrRetToBuf(&strret, leaf, name, MAX_PATH);
}

DWORD_PTR ILFileInfo(const ITEMIDLIST* pidl, SHFILEINFO* info, DWORD flags, DWORD dwFileAttr)
{
	return ::SHGetFileInfo((PCWSTR)pidl, dwFileAttr, info, sizeof(SHFILEINFO), flags | SHGFI_PIDL);
}

int ILIconIndex(const ITEMIDLIST* pidl)
{
	SHFILEINFO  file;
	if (!ILFileInfo(pidl, &file, SHGFI_SYSICONINDEX | SHGFI_SMALLICON))
		return I_IMAGENONE;
	return file.iIcon;
}

namespace
{
	HRESULT HresultFromShellExecute(HINSTANCE hInstance, PCWSTR path, PCWSTR verb, HWND hwnd)
	{
		if (hInstance > (HINSTANCE)32)
			return S_OK;
		if (hwnd)
		{	// hwnd が指定された場合のみエラーを表示
			WCHAR buf[1024] = L"";
			switch ((INT_PTR)hInstance)
			{
			case SE_ERR_FNF:
			case SE_ERR_PNF:
				swprintf_s(buf, _S(STR_E_NOENT), path);
				break;
			case SE_ERR_ACCESSDENIED:
			case SE_ERR_SHARE:
				swprintf_s(buf, _S(STR_E_ACCESS), path);
				break;
			case SE_ERR_OOM: // Out of Memory
				str::load(STR_E_NOMEM, buf);
				break;
			case SE_ERR_DLLNOTFOUND:
				swprintf_s(buf, _S(STR_E_NODLL), path);
				break;
			case SE_ERR_ASSOCINCOMPLETE:
			case SE_ERR_NOASSOC:
				swprintf_s(buf, _S(STR_E_NOACTION), path, verb);
				break;
			case SE_ERR_DDETIMEOUT:
			case SE_ERR_DDEFAIL:
			case SE_ERR_DDEBUSY:
				swprintf_s(buf, _S(STR_E_DDE), path);
				break;
			default:
				swprintf_s(buf, _S(STR_E_NOEXEC), path);
				break;
			}
			if (!str::empty(buf))
				Alert(buf, null, null, MB_OK | MB_ICONERROR);
		}
		return HRESULT_FROM_WIN32(GetLastError());
	}

	HRESULT DoPathExecute(PCWSTR path, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd)
	{
		ASSERT(!str::empty(path));

		// path
		WCHAR quoted[MAX_PATH+2];
		// exe
		PCWSTR exe = null;
		WCHAR exepath[MAX_PATH];
		// dir
		WCHAR dirpath[MAX_PATH];
		// verb
		WCHAR verbbuf[MAX_PATH] = L"";
		if (!str::empty(verb))
		{
			ExpandEnvironmentStrings(verb, verbbuf, MAX_PATH);
			verb = verbbuf;
			if (verb[0] == L'.')
			{	// verb = 拡張子
				if FAILED(RegGetAssocExe(verb, exepath))
					return E_INVALIDARG; // 関連付けされたEXEを取得できなかった
				exe = exepath;
			}
			else
			{
				KnownFolderResolve(exepath, verb);
				if (*PathFindExtension(verb) == L'.')
				{	// verb = ファイルパス
					exe = exepath;
				}
			}
		}

		if (str::empty(dir))
		{	// ワーキングディレクトリは、起動するファイルと同じ
			wcscpy_s(dirpath, path);
			if (!PathIsDirectory(dirpath))
				PathRemoveFileSpec(dirpath);
			dir = dirpath;
		}

		if (hwnd)
			hwnd = ::GetAncestor(hwnd, GA_ROOTOWNER);

		HINSTANCE hInstance;
		if (exe)
		{
			if (wcschr(path, L' '))
			{
				size_t ln = wcslen(path);
				quoted[0] = L'\"';
				wcscpy_s(quoted+1, MAX_PATH - 1, path);
				quoted[ln+1] = L'\"';
				quoted[ln+2] = L'\0';
				path = quoted;
			}
#ifdef NOTIMPL
			if (args)
				...;
#endif
			hInstance = ::ShellExecute(hwnd, null, exe, path, dir, SW_SHOWNORMAL);
		}
		else
		{
			hInstance = ::ShellExecute(hwnd, verb, path, args, dir, SW_SHOWNORMAL);
		}
		return HresultFromShellExecute(hInstance, path, verb, hwnd);
	}

	HRESULT DoILExecute(const ITEMIDLIST* pidl, PCWSTR path, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd)
	{
		SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO), SEE_MASK_IDLIST, ::GetAncestor(hwnd, GA_ROOTOWNER) };
		if (!hwnd)
			info.fMask |= SEE_MASK_FLAG_NO_UI;
		info.nShow = SW_SHOWNORMAL;
		info.lpIDList = (ITEMIDLIST*)pidl;
		info.lpDirectory = dir;
		info.lpVerb = verb;
		info.lpParameters = args;
		WCHAR dirpath[MAX_PATH];
		if (str::empty(dir) && !str::empty(path))
		{	// ワーキングディレクトリは、起動するファイルと同じ
			wcscpy_s(dirpath, path);
			PathRemoveFileSpec(dirpath);
			info.lpDirectory = dirpath;
		}
		if (ShellExecuteEx(&info))
			return S_OK;
		return HresultFromShellExecute(info.hInstApp, path, verb, hwnd);
	}

	bool str_replace(WCHAR buf[], size_t bufsize, PCWSTR from, PCWSTR to)
	{
		PWSTR pos = wcsstr(buf, from);
		if (!pos)
			return false;
		size_t ln = wcslen(pos);
		size_t from_ln = wcslen(from);
		size_t to_ln = wcslen(to);
		memmove(pos + to_ln, pos + from_ln, (ln+1) * sizeof(WCHAR));
		memcpy(pos, to, to_ln * sizeof(WCHAR));
		return true;
	}

	bool split_path(PCWSTR* src, WCHAR dst[MAX_PATH])
	{
		if (!src || !*src)
			return false;
		while (iswspace(**src)) { (*src)++; }

		PCWSTR end;
		switch (**src)
		{
		case L'\0':
			return false;
		case L'"':
			end = wcschr((*src) + 1, L'"');
			if (end)
			{
				wcsncpy_s(dst, MAX_PATH, *src, end - *src + 1);
				*src = end + 1;
			}
			else
			{	// クォートミスだが、対処する。
				wcscpy_s(dst, MAX_PATH, *src);
				wcscat_s(dst, MAX_PATH, L"\"");
				*src = null;
			}
			return true;
		default:
			end = *src;
			while (*end && !iswspace(*end)) { end++; }
			wcsncpy_s(dst, MAX_PATH, *src, end - *src);
			*src = end;
			return true;
		}
	}
}

HRESULT ILExecute(const ITEMIDLIST* pidl, PCWSTR verb, PCWSTR args, PCWSTR dir)
{
	if (!pidl)
		return E_INVALIDARG;

	HWND hwnd = GetWindow();

	WCHAR path[MAX_PATH] = L"";

	if SUCCEEDED(ILPath(pidl, path))
	{
		if (!str::empty(verb))
		{	// パスが取得できない場合は、どうせ特殊実行できない。
			HRESULT hr;
			if SUCCEEDED(hr = DoPathExecute(path, verb, args, dir, null))
				return S_OK;
			else
				verb = null; // この動詞では実行できない。
		}

		// ショートカット先を取得する
		ref<IShellLink> link;
		ILPtr			target;
		if SUCCEEDED(LinkResolve(path, &target, &link))
		{
			const size_t BUFSIZE = 65536;
			WCHAR arguments[BUFSIZE];

			if (SUCCEEDED(link->GetArguments(arguments, BUFSIZE)) && !str::empty(arguments))
			{
				// dir
				bool used = str_replace(arguments, BUFSIZE, L"<dir>", dir);

				if (wcsstr(arguments, L"<all>"))
				{	// all
					if (str::empty(args))
						return S_FALSE;
					str_replace(arguments, BUFSIZE, L"<all>", args);
				}
				else if (wcsstr(arguments, L"<each>"))
				{	// each
					if (str::empty(args))
						return S_FALSE;

					WCHAR arg[MAX_PATH];
					PCWSTR a = args;
					while (split_path(&a, arg))
					{
						WCHAR tmp[BUFSIZE];
						wcscpy_s(tmp, arguments);
						if (str_replace(tmp, BUFSIZE, L"<each>", arg))
							DoILExecute(target, path, verb, tmp, dir, hwnd);
					}
					return S_OK;
				}
				else if (!used && !str::empty(args))
				{	// 指定なし。末尾に追加する。
					if (!str::empty(arguments))
						wcscat_s(arguments, L" ");
					wcscat_s(arguments, args);
				}

				return DoILExecute(target, path, verb, arguments, dir, hwnd);
			}
		}
	}

	return DoILExecute(pidl, path, verb, args, dir, hwnd);
}

bool ILEquals(const ITEMIDLIST* lhs, const ITEMIDLIST* rhs) throw()
{
	if (!lhs && !rhs)
		return true;
	else if (!lhs || !rhs)
		return false;
	return ILIsEqual(lhs, rhs) != 0;
}

#define CSIDL_MAX	(CSIDL_DRIVES+1)
static ITEMIDLIST* CACHED_ITEMS[CSIDL_MAX];

const ITEMIDLIST* ILGetCache(UINT csidl)
{
	if (csidl >= CSIDL_MAX)
		return null;
	if (CACHED_ITEMS[csidl] == null)
		::SHGetSpecialFolderLocation(GetWindow(), csidl, &CACHED_ITEMS[csidl]);
	return CACHED_ITEMS[csidl];
}

bool ILEquals(const ITEMIDLIST* item, UINT csidl)
{
	return ILEquals(item, ILGetCache(csidl));
}

bool ILIsDescendant(const ITEMIDLIST* item, UINT csidl)
{
	const ITEMIDLIST* parent = ILGetCache(csidl);
	if (parent == null)
		return false;
	else
		return ILEquals(item, parent) || ILIsParent(parent, item, false);
}

int ILCompare(const ITEMIDLIST* lhs, const ITEMIDLIST* rhs) throw()
{
	if (!lhs && !rhs)
		return 0;
	else if (!lhs)
		return -1;
	else if (!rhs)
		return +1;

	for (;;)
	{
		USHORT ln = lhs->mkid.cb;
		USHORT rn = rhs->mkid.cb;
		if (ln == 0 && rn == 0)
			return 0;
		else if (ln == 0)
			return -1;
		else if (rn == 0)
			return +1;

		USHORT sz = math::min(ln, rn);
		if (int cmp = memcmp(lhs, rhs, sz))
			return cmp;
		else if (ln < rn)
			return -1;
		else if (ln > rn)
			return +1;
		lhs = (ITEMIDLIST*)((BYTE*)lhs + ln);
		rhs = (ITEMIDLIST*)((BYTE*)rhs + rn);
	}
}

//==========================================================================================================================

CIDAPtr CIDACreate(IDataObject* data) throw()
{
	if (!data)
		return null;

	FORMATETC FORMAT_IDLIST = { (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST), null, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM medium;
	if FAILED(data->GetData(&FORMAT_IDLIST, &medium))
		return null;
	CIDA* src = (CIDA*)::GlobalLock(medium.hGlobal);
	size_t size = CIDASize(src);
	CIDAPtr dst(size);
	memcpy(dst, src, size);
	::GlobalUnlock(medium.hGlobal);
	::GlobalFree(medium.hGlobal);
	return dst;
}

CIDAPtr CIDACreate(IShellView* view, int svgio) throw()
{
	if (!view)
		return null;

	// FIXME: XPでは SVGIO_CHECKED が働かないことがある。

	ref<IDataObject> data;
	if FAILED(view->GetItemObject(svgio, IID_PPV_ARGS(&data)))
		return null;

	return CIDACreate(data);
}

CIDAPtr CIDACreate(IFolderView* view, int svgio) throw()
{
	if (!view)
		return null;

	// FIXME: XPでは SVGIO_CHECKED が働かないことがある。

	ref<IDataObject> data;
	if FAILED(view->Items(svgio, IID_PPV_ARGS(&data)))
		return null;

	return CIDACreate(data);
}

CIDAPtr CIDACreate(const ITEMIDLIST* parent, size_t count, PCUITEMID_CHILD_ARRAY children)
{
	size_t size = sizeof(UINT) + ILGetSize(parent);
	for (size_t i = 0; i < count; ++i)
		size += sizeof(UINT) + ILGetSize(children[i]);

	CIDAPtr cida(size);
	cida->cidl = (UINT)count;
	BYTE* buffer = (BYTE*)&cida->aoffset[count + 1];

	size = ILGetSize(parent);
	cida->aoffset[0] = (UINT)(buffer - (BYTE*)cida.ptr);
	memcpy(buffer, parent, size);
	buffer += size;

	for (size_t i = 0; i < count; ++i)
	{
		size = ILGetSize(parent);
		cida->aoffset[i + 1] = (UINT)(buffer - (BYTE*)cida.ptr);
		memcpy(buffer, children[i], size);
		buffer += size;
	}

	return cida;
}

CIDAPtr CIDAClone(const CIDA* items) throw()
{
	size_t size = CIDASize(items);
	CIDAPtr clone(size);
	memcpy(clone, items, size);
	return clone;
}

const ITEMIDLIST* CIDAParent(const CIDA* items) throw()
{
	if (!items)
		return null;
	return (const ITEMIDLIST*)(((BYTE*)items) + items->aoffset[0]);
}

const ITEMIDLIST* CIDAChild(const CIDA* items, size_t index) throw()
{
	if (!items || index >= CIDACount(items))
		return null;
	return (const ITEMIDLIST*)(((BYTE*)items) + items->aoffset[index+1]);
}

size_t CIDASize(const CIDA* items)
{
	ASSERT(items);
	return items->aoffset[items->cidl] + ILGetSize(CIDAChild(items, CIDACount(items)-1));
}

//==========================================================================================================================
