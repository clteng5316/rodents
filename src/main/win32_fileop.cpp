// win32_fileop.cpp

#include "stdafx.h"
#include "ref.hpp"
#include "string.hpp"
#include "stream.hpp"
#include "win32.hpp"

//==========================================================================================================================
// File operations

HRESULT FileOperate(IFileOperation** pp)
{
	HRESULT	hr = CreateInstance(CLSID_FileOperation, pp);
	if FAILED(hr)
		return hr;
	(*pp)->SetOperationFlags(FOF_ALLOWUNDO);
	(*pp)->SetOwnerWindow(GetWindow());
	return S_OK;
}

HRESULT FileRename(IFileOperation* op, IShellItem* item, PCWSTR name)
{
	return op->RenameItem(item, name, null);
}

HRESULT FileRename(IFileOperation* op, IUnknown* items, PCWSTR name)
{
	return op->RenameItems(items, name);
}

#if 0
HRESULT FileRename(IFileOperation* op, PCWSTR item, PCWSTR name)
{
	HRESULT hr;
	ref<IFileOperationXp> xp;
	if SUCCEEDED(op->QueryInterface(&xp))
		return xp->RenameItem(item, name);
	ref<IShellItem> path;
	if FAILED(hr = PathCreate(&path, item))
		return hr;
	return FileRename(op, path, name);
}
#endif

HRESULT FileRename(IFileOperation* op, size_t num, PCWSTR items[], PCWSTR name);

HRESULT FileMove(IFileOperation* op, IShellItem* item, IShellItem* folder, PCWSTR name)
{
	return op->MoveItem(item, folder, name, null);
}

HRESULT FileMove(IFileOperation* op, IUnknown* items, IShellItem* folder)
{
	return op->MoveItems(items, folder);
}

#if 0
HRESULT FileMove(IFileOperation* op, PCWSTR item, PCWSTR path)
{
	HRESULT hr;
	ref<IFileOperationXp> xp;
	if SUCCEEDED(op->QueryInterface(&xp))
		return xp->MoveItem(item, path);
	ref<IShellItem> src, dir;
	PCWSTR name;
	if FAILED(hr = CreateShellItems(item, path, &src, &dir, &name))
		return hr;
	return FileMove(op, src, dir, name);
}
#endif

HRESULT FileMove(IFileOperation* op, size_t num, PCWSTR items[], PCWSTR folder);

HRESULT FileCopy(IFileOperation* op, IShellItem* item, IShellItem* folder, PCWSTR name)
{
	return op->CopyItem(item, folder, name, null);
}

HRESULT FileCopy(IFileOperation* op, IUnknown* items, IShellItem* folder)
{
	return op->CopyItems(items, folder);
}

#if 0
HRESULT FileCopy(IFileOperation* op, PCWSTR item, PCWSTR path)
{
	HRESULT hr;
	ref<IFileOperationXp> xp;
	if SUCCEEDED(op->QueryInterface(&xp))
		return xp->CopyItem(item, path);
	ref<IShellItem> src, dir;
	PCWSTR name;
	if FAILED(hr = CreateShellItems(item, path, &src, &dir, &name))
		return hr;
	return FileCopy(op, src, dir, name);
}
#endif

HRESULT FileCopy(IFileOperation* op, size_t num, PCWSTR items[], PCWSTR folder);

HRESULT FileDelete(IFileOperation* op, IShellItem* item)
{
	return op->DeleteItem(item, null);
}

HRESULT FileDelete(IFileOperation* op, IUnknown* items)
{
	return op->DeleteItems(items);
}

#if 0
HRESULT FileDelete(IFileOperation* op, PCWSTR item)
{
	HRESULT hr;
	ref<IFileOperationXp> xp;
	if SUCCEEDED(op->QueryInterface(&xp))
		return xp->DeleteItem(item);
	ref<IShellItem> path;
	if FAILED(hr = PathCreate(&path, item))
		return hr;
	return FileDelete(op, path);
}

HRESULT FileDelete(IFileOperation* op, size_t num, PCWSTR items[])
{
	ref<IFileOperationXp> xp;
	if SUCCEEDED(op->QueryInterface(&xp))
		return xp->DeleteItems(num, items);
	else if (ref<IShellItemArray> arr = ShellItemArrayCreate(num, items))
		return op->DeleteItems(arr);
	else
		return E_FAIL;
}
#endif

HRESULT FileNew(IFileOperation* op, IShellItem* folder, PCWSTR name) throw()
{
	// (folder, attr, name, pszTemplateName, sink);
	return op->NewItem(folder, 0, name, null, null);
}

#if 0
HRESULT FileNew(IFileOperation* op, PCWSTR path) throw()
{
	HRESULT hr;
	ref<IFileOperationXp> xp;
	if SUCCEEDED(op->QueryInterface(&xp))
		return xp->NewItem(path);

	ref<IShellItem> item;
	PCWSTR name = PathFindFileName(path);
	WCHAR dir[MAX_PATH];
	wcsncpy_s(dir, path, name - path);
	if FAILED(hr = PathCreate(&item, dir))
		return hr;
	return FileNew(op, item, name);
}
#endif

//================================================================================

namespace
{
	static bool IsCopy()
	{
		static const UINT CF_PREFERREDDROPEFFECT = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
		HGLOBAL hDropEffect = ::GetClipboardData(CF_PREFERREDDROPEFFECT);
		DWORD* pdw = reinterpret_cast<DWORD*>(GlobalLock(hDropEffect));
		ASSERT(pdw);
		DWORD dwEffect = (pdw ? *pdw : DROPEFFECT_COPY);
		GlobalUnlock(hDropEffect);
		return (dwEffect & DROPEFFECT_COPY) != 0;
	}

	static HRESULT FileCutOrCopy(PCWSTR srcSingle, DWORD dwDropEffect)
	{
		if (!srcSingle)
			return E_INVALIDARG;
		size_t len = lstrlen(srcSingle);
		if (len == 0)
			return E_INVALIDARG;

		//CF_HDROPを作成
		DROPFILES dropfiles = { sizeof(DROPFILES), 0, 0, true, TRUE };
		HGLOBAL hDrop;
		try
		{
			ref<IStream> stream;
			StreamCreate(&stream, &hDrop);
			StreamWrite(stream, &dropfiles, sizeof(DROPFILES));
			StreamWrite(stream, srcSingle , (len+1)*sizeof(WCHAR));
			StreamWrite(stream, L"\0"  , sizeof(WCHAR));
		}
		catch (HRESULT hr)
		{
			::GlobalFree(hDrop);
			return hr;
		}

		// Preferred DropEffectを作成
		HGLOBAL hDropEffect = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
		DWORD* pdw = static_cast<DWORD*>(GlobalLock(hDropEffect));
		*pdw = dwDropEffect;
		GlobalUnlock(hDropEffect);

		// クリップボードにデーターをセット
		HWND hwnd = GetWindow();
		UINT CF_PREFERREDDROPEFFECT = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
		if (!::OpenClipboard(hwnd))
		{
			::GlobalFree(hDrop);
			::GlobalFree(hDropEffect);
			return HRESULT_FROM_WIN32(GetLastError());
		}
		::EmptyClipboard();
		::SetClipboardData(CF_HDROP, hDrop);
		::SetClipboardData(CF_PREFERREDDROPEFFECT, hDropEffect);
		::CloseClipboard();
		return S_OK;
	}
}

HRESULT FilePaste(PCWSTR dst)
{
	if (::IsClipboardFormatAvailable(CF_HDROP) &&
		::PathIsDirectory(dst) &&
		::OpenClipboard(NULL))
	{
		if (HDROP hDrop = (HDROP)::GetClipboardData(CF_HDROP))
		{
			bool copy = IsCopy();

			StringBuffer srcfiles;
			const UINT count = ::DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			for (UINT i = 0; i < count; ++i)
			{
				WCHAR path[MAX_PATH];
				::DragQueryFile(hDrop, i, path, MAX_PATH);
				srcfiles.append(path, wcslen(path) + 1);
			}

			if (!copy)
				::EmptyClipboard();
			::CloseClipboard();

			if (!srcfiles.empty())
			{
				srcfiles.terminate();
				return FileOperate((copy ? FO_COPY : FO_MOVE), FOF_ALLOWUNDO, srcfiles, dst);
			}
			return S_OK;
		}
		::CloseClipboard();
	}
	return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT FileCut(PCWSTR src)
{
	return FileCutOrCopy(src, DROPEFFECT_MOVE);
}

HRESULT FileCopy(PCWSTR src)
{
	return FileCutOrCopy(src, DROPEFFECT_COPY | DROPEFFECT_LINK);
}

//================================================================================

namespace
{
	static bool IsEqualClassName(HWND hwnd, PCWSTR clsname)
	{
		if (!hwnd)
			return false;
		WCHAR buffer[MAX_PATH] = L"";
		::GetClassName(hwnd, buffer, MAX_PATH);
		return wcscmp(buffer, clsname) == 0;
	}
	static BOOL CALLBACK EnumFileDialog(HWND hwnd, LPARAM lParam)
	{
		HWND hComboBox = NULL;
		if (!::IsWindowVisible(hwnd))
			return TRUE;
		if (IsEqualClassName(hwnd, L"#32770")
		&& IsEqualClassName(::GetDlgItem(hwnd, 0x00000461), L"SHELLDLL_DefView")
		&& IsEqualClassName(hComboBox = ::GetDlgItem(hwnd, 0x0000047C), L"ComboBoxEx32"))
		{
			// トップレベル ファイルダイアログ
			*(HWND*)lParam = hComboBox;
			return FALSE;
		}

		// ファイルダイアログではないので子供を捜す
		if (HWND hwndChild = FindWindowEx(hwnd, null, L"#32770", null))
		{
			if (IsEqualClassName(::GetDlgItem(hwndChild, 0x00000461), L"SHELLDLL_DefView")
			&& IsEqualClassName(hComboBox = ::GetDlgItem(hwndChild, 0x0000047C), L"ComboBoxEx32"))
			{
				*(HWND*)lParam = hComboBox;
				return FALSE;
			}
		}
        return TRUE;
	}
	static HWND FindPathEditOfFileDialog()
	{
		HWND hwnd = null;
		EnumWindows(EnumFileDialog, (LPARAM)&hwnd);
		return hwnd;
	}
}

HRESULT FileDialogSetPath(PCWSTR path)
{
	HWND hComboBox = FindPathEditOfFileDialog();
	if (!hComboBox)
		return E_FAIL;

	WCHAR orig[MAX_PATH];
	::SendMessage(hComboBox, WM_GETTEXT, MAX_PATH, (LPARAM)orig);
	::SendMessage(hComboBox, WM_SETTEXT, 0, (LPARAM)path);
	::PostMessage(hComboBox, WM_KEYDOWN, VK_RETURN, 0);
	HWND dlg = ::GetAncestor(hComboBox, GA_ROOT);
	::BringWindowToTop(dlg);
	::SetActiveWindow(dlg);
	::SendMessage(hComboBox, WM_SETTEXT, 0, (LPARAM)orig);
	return S_OK;
}
