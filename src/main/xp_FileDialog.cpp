// xp_filedialog.cpp

#include "stdafx.h"
#include "unknown.hpp"
#include "win32.hpp"
#include <commdlg.h>

#pragma comment(lib, "comdlg32")

//==========================================================================================================================

namespace
{
	class XpFileDialog :
		public Unknown< implements<IModalWindow, IFileDialog> >
	{
	private:
		OPENFILENAME	m_ofn; 
		WCHAR			m_title[_MAX_FNAME];
		WCHAR			m_path[_MAX_PATH];
		WCHAR			m_extension[_MAX_EXT];

	public:
		XpFileDialog();

	protected:
		virtual BOOL	DoModal(OPENFILENAME* ofn) = 0;

	public: // IModalWindow
		STDMETHODIMP Show(HWND hwndParent);

	public: // IFileDialog
		STDMETHODIMP SetFileTypes(UINT cFileTypes, const COMDLG_FILTERSPEC* rgFilterSpec)	{ return E_NOTIMPL; }
		STDMETHODIMP SetFileTypeIndex(UINT iFileType)		{ return E_NOTIMPL; }
		STDMETHODIMP GetFileTypeIndex(UINT* piFileType)		{ return E_NOTIMPL; }
		STDMETHODIMP Advise(IFileDialogEvents* events, DWORD* cookie)	{ return E_NOTIMPL; }
		STDMETHODIMP Unadvise(DWORD cookie)					{ return E_NOTIMPL; }
		STDMETHODIMP SetOptions(DWORD fos)					{ return E_NOTIMPL; }
		STDMETHODIMP GetOptions(DWORD* pfos)				{ return E_NOTIMPL; }
		STDMETHODIMP SetDefaultFolder(IShellItem* item)		{ return E_NOTIMPL; }
		STDMETHODIMP SetFolder(IShellItem* item)			{ return E_NOTIMPL; }
		STDMETHODIMP GetFolder(IShellItem** pp)			{ return E_NOTIMPL; }
		STDMETHODIMP GetCurrentSelection(IShellItem** pp)	{ return E_NOTIMPL; }
		STDMETHODIMP SetFileName(PCWSTR name)				{ return E_NOTIMPL; }
		STDMETHODIMP GetFileName(LPWSTR* name)				{ return E_NOTIMPL; }
		STDMETHODIMP SetTitle(PCWSTR pszTitle)				{ return E_NOTIMPL; }
		STDMETHODIMP SetOkButtonLabel(PCWSTR label)		{ return E_NOTIMPL; }
		STDMETHODIMP SetFileNameLabel(PCWSTR label)		{ return E_NOTIMPL; }
		STDMETHODIMP GetResult(IShellItem** pp);
		STDMETHODIMP AddPlace(IShellItem* item, FDAP fdap)	{ return E_NOTIMPL; }
		STDMETHODIMP SetDefaultExtension(PCWSTR extension);
		STDMETHODIMP Close(HRESULT hr)						{ return E_NOTIMPL; }
		STDMETHODIMP SetClientGuid(REFGUID guid)			{ return E_NOTIMPL; }
		STDMETHODIMP ClearClientData(void)					{ return E_NOTIMPL; }
		STDMETHODIMP SetFilter(IShellItemFilter* pFilter)	{ return E_NOTIMPL; }

	public: // IFileOpenDialog
		STDMETHODIMP GetResults(IShellItemArray** items)		{ return E_NOTIMPL; }
		STDMETHODIMP GetSelectedItems(IShellItemArray** items)	{ return E_NOTIMPL; }

	public: // IFileSaveDialog
		STDMETHODIMP SetSaveAsItem(IShellItem* item)		{ return E_NOTIMPL; }
		STDMETHODIMP SetProperties(IPropertyStore* store)	{ return E_NOTIMPL; }
		STDMETHODIMP SetCollectedProperties(IPropertyDescriptionList* pList, BOOL fAppendDefault){ return E_NOTIMPL; }
		STDMETHODIMP GetProperties(IPropertyStore** ppStore){ return E_NOTIMPL; }
		STDMETHODIMP ApplyProperties(IShellItem* item, IPropertyStore* store, HWND hwnd, IFileOperationProgressSink* pSink){ return E_NOTIMPL; }
	};

	class XpFileOpenDialog : public XpFileDialog
	{
	protected:
		BOOL	DoModal(OPENFILENAME* ofn)	{ return ::GetOpenFileName(ofn); }
	};

	class XpFileSaveDialog : public XpFileDialog
	{
	protected:
		BOOL	DoModal(OPENFILENAME* ofn)	{ return ::GetSaveFileName(ofn); }
	};
}

//==========================================================================================================================

UINT_PTR CALLBACK XpFileDialog_OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		CenterWindow(::GetParent(hwnd)); // 親を取得するのがポイント！
		break;
	}

	return 0;
}

XpFileDialog::XpFileDialog()
{
	memset(&m_ofn, 0, sizeof(m_ofn)); // initialize structure to 0/null
	m_path[0] = L'\0';
	m_title[0] = L'\0';

	m_ofn.lStructSize = sizeof(m_ofn);
	m_ofn.lpstrFile = m_path;
	m_ofn.nMaxFile = _MAX_PATH;
	m_ofn.lpstrFileTitle = m_title;
	m_ofn.nMaxFileTitle = _MAX_FNAME;
	m_ofn.Flags |= OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
	m_ofn.lpstrFilter = null; //  lpszFilter;
	m_ofn.hInstance = ::GetModuleHandle(null);
	m_ofn.lpfnHook = XpFileDialog_OnMessage;
//	m_ofn.hwndOwner = hWndParent;

	// setup initial file name
//	if (lpszFileName != null)
//		lstrcpyn(m_path, lpszFileName, _MAX_PATH);
}

STDMETHODIMP XpFileDialog::Show(HWND hwndParent)
{
//	ASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
//	ASSERT(m_ofn.lpfnHook != null);	// can still be a user hook
//	ASSERT(m_ofn.Flags & OFN_EXPLORER);

	m_ofn.hwndOwner = hwndParent;
	if (!m_ofn.hwndOwner)
		m_ofn.hwndOwner = ::GetActiveWindow();

//	ASSERT(hwnd == null);
//	_AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBase*)this);
	BOOL r = DoModal(&m_ofn);
//	hwnd = null;

	return r ? S_OK : S_FALSE;
}

STDMETHODIMP XpFileDialog::SetDefaultExtension(PCWSTR extension)
{
	if (extension)
	{
		m_ofn.lpstrDefExt = m_extension;
		wcscpy_s(m_extension, extension);
	}
	else
		m_ofn.lpstrDefExt = null;
	return S_OK;
}

STDMETHODIMP XpFileDialog::GetResult(IShellItem** pp)
{
	return PathCreate(pp, m_path);
}

//==========================================================================================================================

HRESULT XpFileOpenDialogCreate(REFINTF pp)
{
	return newobj<XpFileOpenDialog>(pp);
}

HRESULT XpFileSaveDialogCreate(REFINTF pp)
{
	return newobj<XpFileSaveDialog>(pp);
}
