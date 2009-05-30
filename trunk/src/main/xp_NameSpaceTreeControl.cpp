// xp_NameSpaceTreeControl.cpp

#include "stdafx.h"
#include "hwnd.hpp"
#include "unknown.hpp"
#include "win32_commctrl.hpp"

//==========================================================================================================================

namespace
{
	class XpNameSpaceTreeControl :
		public Unknown< implements<INameSpaceTreeControl, IOleWindow, IObjectWithSite> >,
		public TreeViewH
	{
	private:
		ref<IServiceProvider>				m_site;
		ref<INameSpaceTreeControlEvents>	m_handler;

	private:
		LRESULT onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK onMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);

	public: // INameSpaceTreeControl
        IFACEMETHODIMP Initialize(HWND hwndParent, RECT* rc, NSTCSTYLE nsctsFlags);
        IFACEMETHODIMP TreeAdvise(IUnknown *punk, DWORD *pdwCookie);
        IFACEMETHODIMP TreeUnadvise(DWORD dwCookie);
        IFACEMETHODIMP AppendRoot(IShellItem* item, SHCONTF grfEnumFlags, NSTCROOTSTYLE grfRootStyle, IShellItemFilter *filter);
        IFACEMETHODIMP InsertRoot(int iIndex, IShellItem* item, SHCONTF grfEnumFlags, NSTCROOTSTYLE grfRootStyle, IShellItemFilter *filter);
        IFACEMETHODIMP RemoveRoot(IShellItem* item);
        IFACEMETHODIMP RemoveAllRoots();
        IFACEMETHODIMP GetRootItems(IShellItemArray** items);
        IFACEMETHODIMP SetItemState(IShellItem* item, NSTCITEMSTATE nstcisMask, NSTCITEMSTATE nstcisFlags);
        IFACEMETHODIMP GetItemState(IShellItem* item, NSTCITEMSTATE nstcisMask, NSTCITEMSTATE *pnstcisFlags);
        IFACEMETHODIMP GetSelectedItems(IShellItemArray** items);
        IFACEMETHODIMP GetItemCustomState(IShellItem* item, int *piStateNumber);
        IFACEMETHODIMP SetItemCustomState(IShellItem* item, int iStateNumber);
        IFACEMETHODIMP EnsureItemVisible(IShellItem* item);
        IFACEMETHODIMP SetTheme(PCWSTR pszTheme);
        IFACEMETHODIMP GetNextItem(IShellItem* item, NSTCGNI nstcgi, IShellItem **ppsiNext);
        IFACEMETHODIMP HitTest(POINT* pt, IShellItem** pp);
        IFACEMETHODIMP GetItemRect(IShellItem* item, RECT* rc);
        IFACEMETHODIMP CollapseAll();

	public: // IOleWindow
		IFACEMETHODIMP GetWindow(HWND* phwnd);
		IFACEMETHODIMP ContextSensitiveHelp(BOOL fEnterMode)	{ return E_NOTIMPL; }

	public: // IObjectWithSite
		IFACEMETHODIMP SetSite(IUnknown* site);
	    IFACEMETHODIMP GetSite(REFIID iid, void** pp);
	};
}

//==========================================================================================================================

LRESULT CALLBACK XpNameSpaceTreeControl::onMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	return ((XpNameSpaceTreeControl*)data)->onMessage(msg, wParam, lParam);
}

LRESULT XpNameSpaceTreeControl::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCDESTROY:
	{
		LRESULT lResult = ::DefSubclassProc(m_hwnd, msg, wParam, lParam);
		Release();
		return lResult;
	}
	default:
		break;
	}
	return ::DefSubclassProc(m_hwnd, msg, wParam, lParam);
}

//==========================================================================================================================
// INameSpaceTreeControl

IFACEMETHODIMP XpNameSpaceTreeControl::Initialize(HWND hwndParent, RECT* rc, NSTCSTYLE nsctsFlags)
{
	InitCommonControls(ICC_TREEVIEW_CLASSES);

	m_hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_TREEVIEW,
		null,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		RECT_XYWH(*rc),
		hwndParent,
		null,
		::GetModuleHandle(null),
		null);

	ASSERT(m_hwnd);
	if (::SetWindowSubclass(m_hwnd, &XpNameSpaceTreeControl::onMessage, 0, (DWORD_PTR) this))
		AddRef();

	return S_OK;
}

IFACEMETHODIMP XpNameSpaceTreeControl::TreeAdvise(IUnknown *punk, DWORD *pdwCookie)
{
	if (m_handler)
		return E_FAIL;
	if (!punk)
		return E_POINTER;
	if (punk->QueryInterface(&m_handler))
		return E_NOINTERFACE;
	if (pdwCookie)
		*pdwCookie = 0xDEADBEEF;
	return S_OK;
}

IFACEMETHODIMP XpNameSpaceTreeControl::TreeUnadvise(DWORD dwCookie)
{
	if (dwCookie != 0xDEADBEEF)
		return E_INVALIDARG;
	m_handler = null;
	return S_OK;
}

IFACEMETHODIMP XpNameSpaceTreeControl::AppendRoot(IShellItem* item, SHCONTF grfEnumFlags, NSTCROOTSTYLE grfRootStyle, IShellItemFilter *filter)
{
	if (HTREEITEM handle = InsertItem(TVI_ROOT, item, LPSTR_TEXTCALLBACK, I_IMAGECALLBACK))
	{
		item->AddRef();
		// Store(item -> { grfEnumFlags, filter } )
//grfRootStyle
//NSTCRS_VISIBLE | NSTCRS_EXPANDED
		return S_OK;
	}
	else
		return E_FAIL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::InsertRoot(int iIndex, IShellItem* item, SHCONTF grfEnumFlags, NSTCROOTSTYLE grfRootStyle, IShellItemFilter *filter)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::RemoveRoot(IShellItem* item)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::RemoveAllRoots()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetRootItems(IShellItemArray** items)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::SetItemState(IShellItem* item, NSTCITEMSTATE nstcisMask, NSTCITEMSTATE nstcisFlags)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetItemState(IShellItem* item, NSTCITEMSTATE nstcisMask, NSTCITEMSTATE *pnstcisFlags)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetSelectedItems(IShellItemArray** items)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetItemCustomState(IShellItem* item, int *piStateNumber)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::SetItemCustomState(IShellItem* item, int iStateNumber)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::EnsureItemVisible(IShellItem* item)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::SetTheme(PCWSTR pszTheme)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetNextItem(IShellItem* item, NSTCGNI nstcgi, IShellItem **ppsiNext)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::HitTest(POINT* pt, IShellItem** pp)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetItemRect(IShellItem* item, RECT* rc)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::CollapseAll()
{
	return E_NOTIMPL;
}

//==========================================================================================================================
// IOleWindow

IFACEMETHODIMP XpNameSpaceTreeControl::GetWindow(HWND* phwnd)
{
	if (!phwnd)
		return E_POINTER;
	*phwnd = m_hwnd;
	return S_OK;
}

//==========================================================================================================================
// IObjectWithSite

IFACEMETHODIMP XpNameSpaceTreeControl::SetSite(IUnknown* site)
{
	m_site = null;
	return site ? site->QueryInterface(&m_site) : S_OK;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetSite(REFIID iid, void** pp)
{
	return m_site.copyto(iid, pp);
}

//==========================================================================================================================

HRESULT XpNameSpaceTreeControlCreate(REFINTF pp)
{
	return newobj<XpNameSpaceTreeControl>(pp);
}
