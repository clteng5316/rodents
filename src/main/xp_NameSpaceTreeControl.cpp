// xp_NameSpaceTreeControl.cpp

#include "stdafx.h"
#include "hwnd.hpp"
#include "unknown.hpp"
#include "win32_commctrl.hpp"
#include "resource.h"
#include "string.hpp"

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
		SHCONTF		m_enumFlags;

	private:
		LRESULT onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
		bool	onNotify(NMHDR* nm, LRESULT& lResult);
		void	onClick(NSTCECLICKTYPE nstcect, int x, int y);
		static LRESULT CALLBACK onMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
		static LRESULT CALLBACK onParentMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
		using TreeViewH::HitTest;

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

LRESULT CALLBACK XpNameSpaceTreeControl::onParentMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	if (msg == WM_NOTIFY)
	{
		NMHDR* nm = (NMHDR*)lParam;
		XpNameSpaceTreeControl* self = (XpNameSpaceTreeControl*)data;
		if (nm->hwndFrom == self->m_hwnd)
		{
			LRESULT lResult = 0;
			if (self->onNotify(nm, lResult))
				return lResult;
		}
	}
	return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT XpNameSpaceTreeControl::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MBUTTONDOWN:
		onClick(NSTCECT_MBUTTON, GET_XY_LPARAM(lParam));
		return 0;
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

bool XpNameSpaceTreeControl::onNotify(NMHDR* nm, LRESULT& lResult)
{
	switch (nm->code)
	{
	case TVN_GETDISPINFO:
	{
		NMTVDISPINFO* disp = (NMTVDISPINFO*) nm;
		TVITEM& item = disp->item;
		IShellItem* path = (IShellItem*) item.lParam;
		if (item.mask & TVIF_TEXT)
		{
			CoStr name;
			if SUCCEEDED(path->GetDisplayName(SIGDN_PARENTRELATIVE, &name))
				wcscpy_s(item.pszText, item.cchTextMax, name);
			else
				wcscpy_s(item.pszText, item.cchTextMax, _S(STR_DESKTOP));
		}
		if (item.mask & (TVIF_SELECTEDIMAGE | TVIF_IMAGE))
		{
			item.iImage = item.iSelectedImage = ILIconIndex(ILCreate(path));
		}
		if (item.mask & TVIF_CHILDREN)
		{
			item.cChildren = 0;
			ref<IEnumShellItems> e;
			if SUCCEEDED(PathChildren(path, &e))
			{
				ref<IShellItem> child;
				while (child = null, e->Next(1, &child, NULL) == S_OK)
				{
					SFGAOF attr = 0;
					if SUCCEEDED(child->GetAttributes(SFGAO_FOLDER | SFGAO_HIDDEN, &attr))
					{
						if ((m_enumFlags & SHCONTF_INCLUDEHIDDEN) == 0 && (attr & SFGAO_HIDDEN))
							continue;
						if (attr & SFGAO_FOLDER)
						{
							if ((m_enumFlags & SHCONTF_FOLDERS) == 0)
								continue;
						}
						else
						{
							if ((m_enumFlags & SHCONTF_NONFOLDERS) == 0)
								continue;
						}

						if (InsertItem(item.hItem, child))
						{
							item.cChildren = 1;
							child->AddRef();
						}
					}
				}
			}
		}
		item.mask |= TVIF_DI_SETITEM;
		return true;
	}
	default:
		return false;
	}
}

void XpNameSpaceTreeControl::onClick(NSTCECLICKTYPE nstcect, int x, int y)
{
	if (!m_handler)
		return;

	TVHITTESTINFO hit = { x, y };
	if (HTREEITEM handle = HitTest(&hit))
	{
		if (handle != GetSelection())
		{
			if (nstcect == NSTCECT_MBUTTON)
				SelectItem(handle);
			IShellItem* item = (IShellItem*) GetItemData(handle);
			// TVHT and NSTCEHT are compatible.
			m_handler->OnItemClick(item, hit.flags, nstcect);
		}
	}
}

//==========================================================================================================================
// INameSpaceTreeControl

IFACEMETHODIMP XpNameSpaceTreeControl::Initialize(HWND hwndParent, RECT* rc, NSTCSTYLE nsctsFlags)
{
	if (!::IsWindow(hwndParent))
		return E_INVALIDARG;

	InitCommonControls(ICC_TREEVIEW_CLASSES);

	m_hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_TREEVIEW,
		null,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
		RECT_XYWH(*rc),
		hwndParent,
		null,
		::GetModuleHandle(null),
		null);

	ASSERT(m_hwnd);
	if (::SetWindowSubclass(m_hwnd, &XpNameSpaceTreeControl::onMessage, 0, (DWORD_PTR) this))
		AddRef();
	::SetWindowSubclass(hwndParent, &XpNameSpaceTreeControl::onParentMessage, 0, (DWORD_PTR) this);

	SetImageList(SHGetImageList(16), TVSIL_NORMAL);

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
	HTREEITEM handle = InsertItem(TVI_ROOT, item);

	if (!handle)
		return E_FAIL;

	item->AddRef();

	// TODO: have separate enum flags for each root
	m_enumFlags = grfEnumFlags;

	if (grfRootStyle & NSTCRS_EXPANDED)
		Expand(handle, TVE_EXPAND);
	if (grfRootStyle & NSTCRS_VISIBLE)
		EnsureVisible(handle);
	return S_OK;
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
