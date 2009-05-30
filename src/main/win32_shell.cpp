// shell.cpp

#include "stdafx.h"
#include "math.hpp"
#include "stream.hpp"
#include "string.hpp"
#include "ref.hpp"
#include "win32.hpp"

//==========================================================================================================================

#define INVALID_FUNCTION_POINTER	((void*)(UINT_PTR)-1)

// この関数は排他制御を行わなくて良い。
// 無駄にはなるが、どうせ同じポインタが取れるので。
bool DynamicLoadVoidPtr(void** fn, PCWSTR module, PCSTR name, int index) throw()
{
	ASSERT(fn);
	if (*fn == INVALID_FUNCTION_POINTER)
		return false;
	if (*fn)
		return true;
	HINSTANCE hDLL = DllLoad(module);
	*fn = ::GetProcAddress(hDLL, name);
	if (!*fn && index > 0)
		*fn = ::GetProcAddress(hDLL, (PCSTR)(INT_PTR)index);
	if (*fn)
		return true;
	*fn = INVALID_FUNCTION_POINTER;
	return false;
}

namespace
{
	static size_t FindLeaf(PCWSTR path, PCWSTR *leaf)
	{
		size_t len;

		*leaf = wcspbrk(path, L"\\/");
		if (*leaf)
		{
			len = *leaf - path;
			while (**leaf == L'\\' || **leaf == L'/')
				++*leaf;
		}
		else
			len = wcslen(path);
		return len;
	}

	static PCWSTR UserKnownFolder(PCWSTR path, PCWSTR* leaf)
	{
		ASSERT(path);
		ASSERT(leaf);

		size_t	len = FindLeaf(path, leaf);
		if (_wcsnicmp(L"ROOT", path, len) == 0)
			return PATH_ROOT;
		else if (_wcsnicmp(L"COMPUTER", path, len) == 0)
			return FOLDER_COMPUTER;
		return null;
	}
}

HRESULT PathCreate(IShellItem** pp, PCWSTR path)
{
	HRESULT				hr;
	ref<IKnownFolder>	known;
	PCWSTR				leaf;
	WCHAR				buffer[MAX_PATH];

	if SUCCEEDED(KnownFolderCreate(&known, path, &leaf))
	{
		if (leaf)
		{
			ref<IShellItem> parent;
			if FAILED(hr = known->GetShellItem(0, IID_PPV_ARGS(&parent)))
				return hr;
			return PathCreate(pp, parent, leaf);
		}
		else
			return known->GetShellItem(0, IID_PPV_ARGS(pp));
	}
	else if (PCWSTR dir = UserKnownFolder(path, &leaf))
	{
		PathCanonicalize(buffer, dir);
		if (leaf)
			PathAppend(buffer, leaf);
	}
	else
		PathCanonicalize(buffer, path);

	for (size_t i = 0; buffer[i]; ++i)
		if (buffer[i] == L'/')
			buffer[i] = L'\\';
	path = buffer;

	static HRESULT (__stdcall *fn)(PCWSTR, IBindCtx*, REFIID, void**) = null;
	if (DynamicLoad(&fn, L"shell32.dll", "SHCreateItemFromParsingName"))
		return fn(path, null, IID_PPV_ARGS(pp));

	// ITEMIDLIST を作成する。
	ILPtr pidl;
	if FAILED(hr = ::SHILCreateFromPath(path, &pidl, null))
		return hr;
	return PathCreate(pp, pidl);
}

HRESULT PathCreate(IShellItem** pp, IShellFolder* folder, const ITEMIDLIST* parent, const ITEMIDLIST* leaf)
{
	if (!pp || !parent || !leaf)
		return E_INVALIDARG;

	static HRESULT (__stdcall *fn)(PCIDLIST_ABSOLUTE, IShellFolder*, PCUITEMID_CHILD, REFIID, void**) = null;
	if (DynamicLoad(&fn, L"shell32.dll", "SHCreateItemWithParent"))
		return fn(parent, folder, leaf, IID_PPV_ARGS(pp));

	return SHCreateShellItem(parent, folder, leaf, pp);
}

HRESULT PathCreate(IShellItem** pp, const ITEMIDLIST* item)
{
	if (!pp || !item)
		return E_INVALIDARG;

	static HRESULT (__stdcall *fn)(PCIDLIST_ABSOLUTE, REFIID, void**) = null;
	if (DynamicLoad(&fn, L"shell32.dll", "SHCreateItemFromIDList"))
		return fn(item, IID_PPV_ARGS(pp));

	return SHCreateShellItem(null, null, item, pp);
}

HRESULT PathCreate(IShellItem** pp, IShellItem* parent, PCWSTR name)
{
	if (!parent)
		return PathCreate(pp, name);

	static HRESULT (__stdcall *fn)(IShellItem*, PCWSTR, IBindCtx*, REFIID, void**) = null;
	if (DynamicLoad(&fn, L"shell32.dll", "SHCreateItemFromRelativeName"))
		return fn(parent, name, null, IID_PPV_ARGS(pp));

	ref<IShellFolder> folder;
	if SUCCEEDED(parent->BindToHandler(null, BHID_SFObject, IID_PPV_ARGS(&folder)))
		if (ILPtr item = ILCreate(parent))
			if (ILPtr leaf = ILCreate(folder, name))
				return PathCreate(pp, folder, item, leaf);
	
	return E_FAIL;
}

HRESULT PathStat(IShellItem* item, WIN32_FIND_DATA* stat) throw()
{
	HRESULT hr;
	CoStr path;
	if SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))
	{
		if (HANDLE handle = FindFirstFile(path, stat))
		{
			FindClose(handle);
			return S_OK;
		}
	}

	ILPtr pidl = ILCreate(item);
	ref<IShellFolder> folder;
	const ITEMIDLIST* leaf;
	if FAILED(hr = ILParentFolder(&folder, pidl, &leaf))
		return hr;
	return SHGetDataFromIDList(folder, leaf, SHGDFIL_FINDDATA, stat, sizeof(WIN32_FIND_DATA));
}

HRESULT PathGetBrowsable(IShellItem* item)
{
	if (!item)
		return E_FAIL;

	SFGAOF flags = 0;
	if (FAILED(item->GetAttributes(SFGAO_FOLDER, &flags)) ||
		(flags & SFGAO_FOLDER) == 0)
		return E_FAIL;

	CoStr path;
	if FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))
		return S_OK;

	if (!path || PathIsDirectory(path))
		return S_OK;	// 仮想フォルダ or ディレクトリ

	PCWSTR ext = PathFindExtension(path);
	WCHAR key[MAX_PATH], def[MAX_PATH], command[MAX_PATH], buffer[MAX_PATH];
	if FAILED(RegGetString(HKEY_CLASSES_ROOT, ext, NULL, key))
		return S_OK; // 未登録項目
	swprintf_s(buffer, L"%s\\shell", key);
	if FAILED(RegGetString(HKEY_CLASSES_ROOT, buffer, NULL, def))
		return S_OK; // エラー
	if (_wcsicmp(def, L"explore") == 0)
		return S_OK; // エクスプローラに関連付け
	if (str::empty(def))
		lstrcpy(def, L"open");
	swprintf_s(buffer, L"%s\\shell\\%s\\command", key, def);
	if FAILED(RegGetString(HKEY_CLASSES_ROOT, buffer, NULL, command))
		return S_OK;

	// 何かアプリケーションに関連付けられていた。
	// 閲覧は出来るが、デフォルト実行としてはファイルとして扱うべき。
	return S_FALSE;
}

static HRESULT StreamFromChildren(IStream** pp, IShellItem* item)
{
	HRESULT					hr;
	ref<IShellItem>			child;
	ref<IEnumShellItems>	e;
	if SUCCEEDED(hr = item->BindToHandler(null, BHID_StorageEnum, IID_PPV_ARGS(&e)))
	{
		e->Next(1, &child, null);
		e = null;
		if (!child)
			return E_FAIL;
		return StreamFromChildren(pp, child);
	}
	else
		return StreamCreate(pp, item, STGM_READ | STGM_SHARE_DENY_NONE);
}

const int THUMBNAIL_SIZE = 256;

HBITMAP PathGetThumbnail(IShellItem* item)
{
	if (!item)
		return null;

	SIZE sz = { THUMBNAIL_SIZE, THUMBNAIL_SIZE };
	ref<IShellItemImageFactory> factory;
	HBITMAP bitmap = null;

	// サイズの制御が自由なので、いまのところ GDI+ を優先させている。
	// 性能を重要視するなら IShellItemImageFactory を優先すべきかもしれない。

	ref<IStream> stream;
	if (SUCCEEDED(StreamFromChildren(&stream, item)) &&
		(bitmap = BitmapLoad(stream, &sz)) != null)
		return bitmap;

	if (SUCCEEDED(item->QueryInterface(&factory)) && 
		SUCCEEDED(factory->GetImage(sz, SIIGBF_BIGGERSIZEOK | SIIGBF_THUMBNAILONLY, &bitmap)))
		return bitmap;

	return null;
}

//==========================================================================================================================
// IShellItemArray

/*
[Shell32.dll]
SHCreateShellItemArrayFromIDLists(UINT cidl, const ITEMIDLIST* rgpidl, IShellItemArray **pp);
SHCreateShellItemArrayFromShellItem(IShellItem* psi, REFIID iid, void** pp);
*/

HRESULT PathsCreate(IShellItemArray** pp, IDataObject* data)
{
	if (!data || !pp)
		return E_POINTER;
	*pp = null;

	static HRESULT (__stdcall *fn)(IDataObject*, REFIID, void**) = null;
	if (DynamicLoad(&fn, L"shell32.dll", "SHCreateShellItemArrayFromDataObject"))
		return fn(data, IID_PPV_ARGS(pp));

	return XpCreateShellItemArray(pp, CIDACreate(data), null);
}

HRESULT PathsCreate(IShellItemArray** pp, IShellView* view, int svgio)
{
	HRESULT	hr;
	if (!view || !pp)
		return E_POINTER;
	*pp = null;

	ref<IDataObject> data;
	if FAILED(hr = view->GetItemObject(svgio, IID_PPV_ARGS(&data)))
		return hr;

	return PathsCreate(pp, data);
}

namespace
{
	class ILSort
	{
	private:
		IShellFolder*	m_folder;
	public:
		ILSort(IShellFolder* folder) : m_folder(folder)	{}
		bool operator () (const ITEMIDLIST* lhs, const ITEMIDLIST* rhs) const throw()
		{
			HRESULT hr = m_folder->CompareIDs(0, lhs, rhs);
			return ((short)HRESULT_CODE(hr) < 0);
		}
	};
}

HRESULT PathsCreate(IShellItemArray** pp, const ITEMIDLIST* parent, IShellFolder* folder, IEnumIDList* items)
{
	HRESULT	hr;
	if (!items || !pp)
		return E_POINTER;
	*pp = null;

	std::vector<ITEMIDLIST*> children;
	for (;;)
	{
		const ULONG COUNT = 16;
		ULONG n = (ULONG)children.size();
		children.resize(n + COUNT);
		ULONG fetched = 0;
		hr = items->Next(COUNT, &children[n], &fetched);
		if FAILED(hr)
			goto cleanup;
		children.resize(n + fetched);
		if (fetched == 0)
			break;
	}
	if (children.empty())
		return E_FAIL;

	std::sort(children.begin(), children.end(), ILSort(folder));

	static HRESULT (__stdcall *fn)(const ITEMIDLIST*, IShellFolder*, UINT, PCUITEMID_CHILD_ARRAY, IShellItemArray**) = null;
	if (DynamicLoad(&fn, L"shell32.dll", "SHCreateShellItemArray"))
		hr = fn(parent, folder, (UINT)children.size(), (PCUITEMID_CHILD_ARRAY)&children[0], pp);
	else
		hr = XpCreateShellItemArray(pp, parent, folder, children.size(), &children[0]);

cleanup:
	for (size_t i = 0; i < children.size(); ++i)
		ILFree(children[i]);
	return hr;
}

//==========================================================================================================================

HRESULT KnownFolderCreate(IKnownFolder** pp, PCWSTR path, PCWSTR* leaf)
{
	ASSERT(*leaf);

	*leaf = null;

	if (wcsncmp(path, L"::{", 3) != 0 && PathIsRelative(path))
	{
		WCHAR	buf[MAX_PATH];
		size_t	len = FindLeaf(path, leaf);
		if (*leaf)
		{
			memcpy(buf, path, sizeof(WCHAR) * len);
			buf[len] = '\0';
			path = buf;
		}

		ref<IKnownFolderManager> manager;
		if SUCCEEDED(manager.newobj(CLSID_KnownFolderManager))
		{
			KNOWNFOLDERID	id;
			if (path[0] == L'{' && 
				SUCCEEDED(CLSIDFromString((LPOLESTR)path, &id)))
				return manager->GetFolder(id, pp);
			else
				return manager->GetFolderByName(path, pp);
		}
	}
	return E_FAIL;
}

void KnownFolderResolve(WCHAR dst[MAX_PATH], PCWSTR src)
{
	ref<IKnownFolder>	known;
	PCWSTR				leaf;

	if SUCCEEDED(KnownFolderCreate(&known, src, &leaf))
	{
		CoStr	parent;
		if FAILED(known->GetPath(0, &parent))
			wcscpy_s(dst, MAX_PATH, src);	// よーわからん
		if (leaf)
			PathCombine(dst, parent, leaf);
		else
			wcscpy_s(dst, MAX_PATH, parent);
	}
	else if (PCWSTR name = UserKnownFolder(src, &leaf))
		PathCombine(dst, name, leaf);
	else
		PathCanonicalize(dst, src);

	for (size_t i = 0; dst[i]; ++i)
		if (dst[i] == L'/')
			dst[i] = L'\\';
}

namespace
{
	typedef std::pair<PCWSTR, PCWSTR>	Path2Name;
	typedef std::vector<Path2Name>		KnownFolderMap;

	inline int pathcmp(PCWSTR lhs, PCWSTR rhs, size_t len = SIZE_MAX)
	{
		for (size_t i = 0; i < len; ++i)
		{
			if (!*lhs || !*rhs)
				return (int)*lhs - (int)*rhs;

			WCHAR	lc = towlower(*lhs);
			if (lc == '/' || lc == '/')
				lc = WCHAR_MAX;

			WCHAR	rc = towlower(*rhs);
			if (rc == '/' || rc == '/')
				rc = WCHAR_MAX;

			if (lc != rc)
				return (int)lc - (int)rc;

			++lhs;
			++rhs;
		}
		return 0;
	}

	struct PathLess
	{
		bool operator () (PCWSTR lhs, PCWSTR rhs) const
		{
			return pathcmp(lhs, rhs) < 0;
		}
		bool operator () (const Path2Name& lhs, const Path2Name& rhs) const
		{
			return operator () (lhs.first, rhs.first);
		}
		bool operator () (const Path2Name& lhs, PCWSTR rhs) const
		{
			return operator () (lhs.first, rhs);
		}
		bool operator () (PCWSTR lhs, const Path2Name& rhs) const
		{
			return operator () (lhs, rhs.first);
		}
	};

	static KnownFolderMap	theKnownFolders;
}

static void InitKnownFolders()
{
	if (theKnownFolders.empty())
	{
		theKnownFolders.push_back(Path2Name(PATH_ROOT, L"ROOT"));

		ref<IKnownFolderManager>	manager;
		KNOWNFOLDERID*				IDs;
		UINT						count;

		if (SUCCEEDED(manager.newobj(CLSID_KnownFolderManager)) &&
			SUCCEEDED(manager->GetFolderIds(&IDs, &count)))
		{
			for (UINT i = 0; i < count; ++i)
			{
				ref<IKnownFolder>	folder;
				CoStr				path;
				CoStr				name;
				if (SUCCEEDED(manager->GetFolder(IDs[i], &folder)) &&
					SUCCEEDED(folder->GetPath(0, &path)) &&
					SUCCEEDED(StringFromCLSID(IDs[i], &name)))
				{
					// メモリはとりあえず解放しない
					theKnownFolders.push_back(Path2Name(path.detach(), name.detach()));
				}
			}
		}

		std::sort(theKnownFolders.begin(), theKnownFolders.end(), PathLess());
	}
}

string KnownFolderEncode(PCWSTR path)
{
	InitKnownFolders();

	WCHAR	folder[MAX_PATH];
	wcscpy_s(folder, path);

	do
	{
		KnownFolderMap::const_iterator i = std::upper_bound(theKnownFolders.begin(), theKnownFolders.end(), folder, PathLess());
		if (i != theKnownFolders.begin())
		{
			--i;
			size_t len = wcslen(i->first);
			if (pathcmp(i->first, path, len) == 0)
			{
				PCWSTR	leaf = path + len;
				size_t	enclen = wcslen(i->second);
				size_t	leaflen = wcslen(leaf) + 1;

				// 特殊フォルダがドライブ直下の場合は "C:\" のようにパスに "\" を含む。
				// leaf が "\" から始まるよう、1文字戻す。
				if (i->first[len - 1] == '\\' && leaflen > 1)
				{
					leaf--;
					leaflen++;
				}

				string	r;
				r.realloc((enclen + leaflen) * sizeof(WCHAR));
				memcpy(r, i->second, enclen * sizeof(WCHAR));
				wcscpy_s(r + enclen, leaflen, leaf);
				return r;
			}
		}
	} while (PathRemoveFileSpec(folder));

	return str::dup(path);
}

//==========================================================================================================================

HRESULT LinkResolve(PCWSTR src, ITEMIDLIST** dst, IShellLink** pp) throw()
{
	ASSERT(dst);

	HRESULT hr;
	ref<IShellLink>		link;
	ref<IPersistFile>	persist;
	WCHAR				path[MAX_PATH];

	KnownFolderResolve(path, src);

	hr = E_FAIL;
	if (_wcsicmp(PathFindExtension(path), L".lnk") == 0 &&
		SUCCEEDED(hr = link.newobj(CLSID_ShellLink)) &&
		SUCCEEDED(hr = link->QueryInterface(&persist)) &&
		SUCCEEDED(hr = persist->Load(path, STGM_READ | STGM_SHARE_DENY_NONE)) &&
		SUCCEEDED(hr = link->GetIDList(dst)) &&
		*dst != null)
	{
		if (pp)
			link.copyto(pp);
		return S_OK;
	}
	else
	{
		// たまに S_FALSE でこちらに抜ける場合がある
		if (pp)
			*pp = null;
		*dst = null;
		return FAILED(hr) ? hr : E_FAIL;
	}
}

HRESULT LinkResolve(PCWSTR src, WCHAR dst[MAX_PATH], IShellLink** pp) throw()
{
	HRESULT hr;

	KnownFolderResolve(dst, src);

	if (_wcsicmp(PathFindExtension(dst), L".lnk") != 0)
	{
		hr = S_FALSE;
		if (pp)
			*pp = null;
	}
	else
	{
		ref<IShellLink>		link;
		ref<IPersistFile>	persist;
		SUCCEEDED(hr = link.newobj(CLSID_ShellLink)) &&
		SUCCEEDED(hr = link->QueryInterface(&persist)) &&
		SUCCEEDED(hr = persist->Load(dst, STGM_READ | STGM_SHARE_DENY_NONE)) &&
		SUCCEEDED(hr = link->GetPath(dst, MAX_PATH, null, 0));
		if (pp)
			link.copyto(pp);
	}

	return hr;
}

HRESULT LinkCreate(const ITEMIDLIST* item, PCWSTR dst, IShellLink** pp) throw()
{
	HRESULT hr;
	ref<IShellLink> link;
	ref<IPersistFile> persist;

	// ファイルに対してショートカットを作成する場合は、
	// ワーキングディレクトリを、それを含む親フォルダに設定する。
	WCHAR dir[MAX_PATH] = L"";
	if (SUCCEEDED(ILPath(item, dir)) && !PathIsDirectory(dir))
		PathRemoveFileSpec(dir);

	SUCCEEDED(hr = link.newobj(CLSID_ShellLink)) &&
	SUCCEEDED(hr = link->SetIDList(item)) &&
	(!dir[0] || SUCCEEDED(hr = link->SetWorkingDirectory(dir))) &&
	SUCCEEDED(hr = link->QueryInterface(&persist)) &&
	SUCCEEDED(hr = persist->Save(dst, STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE));

	if (pp)
		link.copyto(pp);
	return hr;
}

//==========================================================================================================================

HIMAGELIST SHGetImageList(int size)
{
	static ref<IImageList> imagelists[SHIL_LAST+1];

	int what;
	if (size < 32)
		what = SHIL_SMALL;
	else if (size < 48)
		what = SHIL_LARGE;
	else if (WINDOWS_VERSION < WINDOWS_VISTA || size < 256)
		what = SHIL_EXTRALARGE;
	else
		what = SHIL_JUMBO;

	ref<IImageList>& image = imagelists[what];

	if (!image)
		SHGetImageList(what, IID_PPV_ARGS(&image));

	// MSDN says: The IImageList pointer type can be cast as an HIMAGELIST as needed.
	return (HIMAGELIST)image.ptr;
}

//==========================================================================================================================

string ClipboardGetText()
{
	string text;
	if (OpenClipboard(NULL))
	{
		if (HANDLE handle = ::GetClipboardData(CF_UNICODETEXT))
		{
			text = str::dup((PCWSTR)::GlobalLock(handle));
			::GlobalUnlock(handle);
		}
		CloseClipboard();
	}
	return text;
}

void ClipboardSetText(PCWSTR text)
{
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		if (!str::empty(text))
		{
			size_t ln = sizeof(WCHAR) * (wcslen(text) + 1);
			HANDLE handle = GlobalAlloc(GMEM_MOVEABLE, ln);
			memcpy(GlobalLock(handle), text, ln);
			GlobalUnlock(handle);
			SetClipboardData(CF_UNICODETEXT, handle);
		}
		CloseClipboard();
	}
}

//==========================================================================================================================

static void NewFileMenuAddMnemonic(HMENU menu)
{
	// フォルダ=F, ショートカット=S は使用済みなので含まない。
	const WCHAR mnemonic[] = L"1234567890ABCDEGHIJKLMNOPQRTUVWXYZ";
	int count = math::min<int>(GetMenuItemCount(menu), 3 + lengthof(mnemonic) - 1);
	int mne = 0;
	for (int i = 3; i < count; ++i)
	{
		WCHAR name[MAX_PATH] = L"&?: ";
		MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_STRING };
		info.dwTypeData = name + 4;
		info.cch = lengthof(name) - 4;
		if (GetMenuItemInfo(menu, i, true, &info) &&
			wcschr(info.dwTypeData, L'&') == null)
		{
			name[1] = mnemonic[mne++];
			info.dwTypeData = name;
			info.cch = lengthof(name);
			SetMenuItemInfo(menu, i, true, &info);
		}
	}
}

HRESULT FileNew(IShellView* view, bool folder)
{
	const UINT	ID_SHELL_FIRST	= 0x1000;
	const UINT	ID_SHELL_LAST	= 0x7FFF;

	HWND hwnd;
	if FAILED(view->GetWindow(&hwnd))
		hwnd = GetWindow();

	ref<IContextMenu2> menu;
	if SUCCEEDED(view->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&menu)))
	{
		MenuH handle = CreatePopupMenu();

		if SUCCEEDED(menu->QueryContextMenu(handle, 0, ID_SHELL_FIRST, ID_SHELL_LAST, CMF_EXPLORE))
		{
			// 通常、下から数えたほうが早いので、逆順に調べる。
			for (int i = GetMenuItemCount(handle) - 1; i >= 0; --i)
			{
				WCHAR name[MAX_PATH];
				MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_STRING | MIIM_SUBMENU };
				info.dwTypeData = name;
				info.cch = lengthof(name);
				// TODO: 新規作成(&W) の文字列をWindowsのリソースから取得すべし。
				if (GetMenuItemInfo(handle, i, true, &info) && info.hSubMenu &&
					(wcscmp(name, L"新規作成(&W)") == 0 || wcscmp(name, L"Ne&w") == 0))
				{
					UINT uID = (UINT)-1;
					menu->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)info.hSubMenu, MAKELPARAM(i, TRUE));
					if (folder)
					{
						uID = GetMenuItemID(info.hSubMenu, 0);
					}
					else
					{
						NewFileMenuAddMnemonic(info.hSubMenu);
						RECT rc;
						::GetWindowRect(hwnd, &rc);
						uID = TrackPopupMenu(info.hSubMenu, TPM_CENTERALIGN | TPM_VCENTERALIGN | TPM_RETURNCMD, RECT_CENTER(rc), 0, hwnd, null);
					}
					if (ID_SHELL_FIRST <= uID && uID <= ID_SHELL_LAST)
					{
						CMINVOKECOMMANDINFO info = { sizeof(CMINVOKECOMMANDINFO) };
						info.hwnd   = hwnd;
						info.lpVerb = MAKEINTRESOURCEA(uID - ID_SHELL_FIRST);
						info.nShow  = SW_SHOWNORMAL;
						// そのままでは作成されたアイテムが名前変更状態にならない。
						// 効率は悪いが、CIDA の変化を調べることで新規アイテムを特定する。
						CIDAPtr items1 = CIDACreate(view, SVGIO_ALLVIEW | SVGIO_FLAG_VIEWORDER);
						CIDAPtr items2;
						if SUCCEEDED(menu->InvokeCommand(&info))
						{
							if (items1 && (items2 = CIDACreate(view, SVGIO_ALLVIEW | SVGIO_FLAG_VIEWORDER)) != null)
							{
								size_t n1 = CIDACount(items1);
								size_t n2 = CIDACount(items2);
								for (int i = 0; n1 == n2 && i < 10; ++i)
								{
									Sleep(100);
									PumpMessage();
								}
								if (n1 < n2)
								{
									for (size_t i = 0; i < n2; ++i)
									{
										const ITEMIDLIST* item = CIDAChild(items2, i);
										if (i >= n1 || !ILIsEqual(item, CIDAChild(items1, i)))
										{
											view->SelectItem(item, SVSI_SELECT | SVSI_FOCUSED | SVSI_EDIT | SVSI_DESELECTOTHERS);
											return S_OK;
										}
									}
								}
							}
							return S_FALSE;
						}
					}
					return E_FAIL;
				}
			}
		}
	}
	return E_FAIL;
}

//==========================================================================================================================

Version::Version() : m_info(null)
{
}

Version::~Version()
{
	close();
}

bool Version::open(PCWSTR filename)
{
	close();
	DWORD dummy;
	DWORD versionInfoSize = ::GetFileVersionInfoSize(filename, &dummy);
	if (versionInfoSize == 0)
		return false;

	struct LangCodePage
	{
		WORD Language;
		WORD CodePage;
	};

#define MAKE_VERSION(hi, lo) \
	((DWORD)HIWORD(hi) << 24 | (DWORD)LOWORD(hi) << 16 | (DWORD)HIWORD(lo) << 8 | LOWORD(lo))

	m_info = new BYTE[versionInfoSize];
	UINT size;
	VS_FIXEDFILEINFO* info;
	if (::GetFileVersionInfo(filename, 0, versionInfoSize, m_info)
	&& ::VerQueryValue(m_info, L"\\", (void**)&info, &size))
	{
		LangCodePage* lang;
		::VerQueryValue(m_info, L"\\VarFileInfo\\Translation", (void**)&lang, &size);
		FileVersion = MAKE_VERSION(info->dwFileVersionMS, info->dwFileVersionLS);
		ProductVersion = MAKE_VERSION(info->dwProductVersionMS, info->dwProductVersionLS);
		m_language = lang->Language;
		m_codepage = lang->CodePage;
	}
	else
	{
		delete[] m_info;
		m_info = NULL;
		return false;
	}
	return true;
}

void Version::close()
{
	delete[] m_info;
	m_info = NULL;
}

PCWSTR Version::value(PCWSTR what) const
{
	PCWSTR value = NULL;
	UINT size;
	WCHAR query[MAX_PATH];
	swprintf_s(query, L"\\StringFileInfo\\%04x%04x\\%s", m_language, m_codepage, what);
	BOOL res = ::VerQueryValue(m_info, query, (void**)&value, &size);
	if (!res)
		return null;
	return value;
}
