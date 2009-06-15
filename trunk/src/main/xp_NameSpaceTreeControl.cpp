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
		void	onInsertItem(HTREEITEM hItem, IShellItem* value);
		void	onDeleteItem(HTREEITEM hItem, IShellItem* value);
		void	onGetDispInfo(NMTVDISPINFO& disp);
		static LRESULT CALLBACK onMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
		static LRESULT CALLBACK onParentMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
		using TreeViewH::HitTest;
		using TreeViewH::GetItemRect;
		using TreeViewH::GetNextItem;
		using TreeViewH::GetItemState;
		using TreeViewH::SetItemState;

	public:
		HTREEITEM	InsertItem(HTREEITEM parent, IShellItem* value);
		HTREEITEM	FindItem(IShellItem* value, bool rootOnly = false);
		void		Collapse(HTREEITEM hItem);
		IShellItem*	GetItemData(HTREEITEM hItem);
		HRESULT		GetItemData(HTREEITEM hItem, IShellItem** pp);

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
        IFACEMETHODIMP GetNextItem(IShellItem* item, NSTCGNI nstcgi, IShellItem **pp);
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
	case TVN_DELETEITEM:
	{
		NMTREEVIEW* nmtv = (NMTREEVIEW*) nm;
		onDeleteItem(nmtv->itemOld.hItem, (IShellItem*) nmtv->itemOld.lParam);
		return true;
	}
//	case TVN_BEGINDRAG		, OnBeginDrag
//	case TVN_BEGINRDRAG	, OnBeginDrag
//	case TVN_KEYDOWN		, OnItemKeyDown
//	case TVN_SELCHANGED	, OnSelChanged)
	case TVN_GETDISPINFO:
		onGetDispInfo(*(NMTVDISPINFO*) nm);
		return true;
	case NM_RETURN:
		// デフォルト動作は「ビープを鳴らす」ので、キャンセルする。
		lResult = 1;
		return true;
	default:
		return false;
	}
}

void XpNameSpaceTreeControl::onClick(NSTCECLICKTYPE nstcect, int x, int y)
{
	if (!m_handler)
		return;

	TVHITTESTINFO hit = { x, y };
	if (HTREEITEM hItem = HitTest(&hit))
	{
		if (hItem != GetSelection())
		{
			if (nstcect == NSTCECT_MBUTTON)
				SelectItem(hItem);
			IShellItem* item = GetItemData(hItem);
			// TVHT and NSTCEHT are compatible.
			m_handler->OnItemClick(item, hit.flags, nstcect);
		}
	}
}

HTREEITEM XpNameSpaceTreeControl::InsertItem(HTREEITEM parent, IShellItem* value)
{
	HTREEITEM hItem = __super::InsertItem(parent, value);
	if (hItem)
		onInsertItem(hItem, value);
	return hItem;
}

// FIXME: このルーチンでは最上位がデスクトップで無いと働かない。
HTREEITEM XpNameSpaceTreeControl::FindItem(IShellItem* value, bool rootOnly)
{
	std::vector< ref<IShellItem> >	items;
	items.push_back(ref<IShellItem>(value));
	for (;;)
	{
		ref<IShellItem> parent;
		if FAILED(items.back()->GetParent(&parent))
		{
			items.pop_back();	// デスクトップは削る
			break;
		}
		items.push_back(parent);
	}

	Expand(TVI_ROOT);
	for (HTREEITEM root = GetNextItem(TVI_ROOT, TVGN_CHILD);
		 root;
		 root = GetNextItem(root, TVGN_NEXT))
	{
		Expand(root);
		std::vector< ref<IShellItem> >::reverse_iterator i = items.rbegin();
		// first child of desktop
		HTREEITEM current = GetNextItem(root, TVGN_CHILD);
		while (current)
		{
			if (IShellItem* item = GetItemData(current))
			{
				int	order;
				if (item->Compare(*i, SICHINT_CANONICAL, &order) == S_OK)
				{
					if (++i == items.rend())
						return current;

					Expand(current);
					current = GetNextItem(current, TVGN_CHILD);
					continue;
				}
			}
			current = GetNextItem(current, TVGN_NEXT);
		}
	}
	return NULL;
}

void XpNameSpaceTreeControl::Collapse(HTREEITEM hItem)
{
	Expand(hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
	TVITEM item = { TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN };
	item.hItem = hItem;
	item.pszText = LPSTR_TEXTCALLBACK;
	item.iImage = item.iSelectedImage = I_IMAGECALLBACK;
	item.cChildren = I_CHILDRENCALLBACK;
	SetItem(item);
}

IShellItem* XpNameSpaceTreeControl::GetItemData(HTREEITEM hItem)
{
	return (IShellItem*) __super::GetItemData(hItem);
}

HRESULT XpNameSpaceTreeControl::GetItemData(HTREEITEM hItem, IShellItem** pp)
{
	if (!pp)
		return E_POINTER;
	*pp = (IShellItem*) GetItemData(hItem);
	if (*pp)
	{
		(*pp)->AddRef();
		return S_OK;
	}
	return E_FAIL;
}

void XpNameSpaceTreeControl::onInsertItem(HTREEITEM hItem, IShellItem* value)
{
	if (value)
		value->AddRef();
}

void XpNameSpaceTreeControl::onDeleteItem(HTREEITEM hItem, IShellItem* value)
{
	if (value)
		value->Release();
}

void XpNameSpaceTreeControl::onGetDispInfo(NMTVDISPINFO& disp)
{
	TVITEM& item = disp.item;
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
						// ZIP書庫もフォルダ扱いになってしまうので、
						// 実ファイルでディレクトリでないものは除外する
						CoStr path;
						if (SUCCEEDED(child->GetDisplayName(SIGDN_FILESYSPATH, &path)) &&
							!PathIsDirectory(path))
							continue;
					}
					else
					{
						if ((m_enumFlags & SHCONTF_NONFOLDERS) == 0)
							continue;
					}

					if (InsertItem(item.hItem, child))
						item.cChildren = 1;
				}
			}
		}
	}
	item.mask |= TVIF_DI_SETITEM;
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
	return InsertRoot(-1, item, grfEnumFlags, grfRootStyle, filter);
}

IFACEMETHODIMP XpNameSpaceTreeControl::InsertRoot(int iIndex, IShellItem* item, SHCONTF grfEnumFlags, NSTCROOTSTYLE grfRootStyle, IShellItemFilter *filter)
{
	ASSERT(!filter);	// not supported
	ASSERT(iIndex < 0);	// not supported

	HTREEITEM hItem = InsertItem(TVI_ROOT, item);
	if (!hItem)
		return E_FAIL;

	// TODO: have separate enum flags for each root
	m_enumFlags = grfEnumFlags;

	if (grfRootStyle & NSTCRS_EXPANDED)
		Expand(hItem);
	if (grfRootStyle & NSTCRS_VISIBLE)
		EnsureVisible(hItem);
	return S_OK;
}

IFACEMETHODIMP XpNameSpaceTreeControl::RemoveRoot(IShellItem* item)
{
	if (HTREEITEM hItem = FindItem(item, true))
	{
		DeleteItem(hItem);
		return S_OK;
	}
	return E_FAIL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::RemoveAllRoots()
{
	DeleteAllItems();
	return S_OK;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetRootItems(IShellItemArray** items)
{
	std::vector<IShellItem*> roots;
	for (HTREEITEM root = GetNextItem(TVI_ROOT, TVGN_CHILD);
		 root;
		 root = GetNextItem(root, TVGN_NEXT))
	{
		roots.push_back(GetItemData(root));
	}
	if (roots.empty())
		return E_FAIL;
	return XpCreateShellItemArray(items, roots.size(), &roots[0]);
}

static DWORD NSTCITEMSTATE2TVIS(NSTCITEMSTATE nstcis)
{
	DWORD	tvis = 0;
	if (nstcis & NSTCIS_SELECTED)	{ tvis |= TVIS_SELECTED; }
	if (nstcis & NSTCIS_EXPANDED)	{ tvis |= TVIS_EXPANDED; }
	if (nstcis & NSTCIS_BOLD)		{ tvis |= TVIS_BOLD; }
	return tvis;
}

static NSTCITEMSTATE TVIS2NSTCITEMSTATE(DWORD tvis)
{
	NSTCITEMSTATE	nstcis = 0;
	if (tvis & TVIS_SELECTED)	{ nstcis |= NSTCIS_SELECTED; }
	if (tvis & TVIS_EXPANDED)	{ nstcis |= NSTCIS_EXPANDED; }
	if (tvis & TVIS_BOLD)		{ nstcis |= NSTCIS_BOLD; }
	return nstcis;
}

IFACEMETHODIMP XpNameSpaceTreeControl::SetItemState(IShellItem* item, NSTCITEMSTATE nstcisMask, NSTCITEMSTATE nstcisFlags)
{
	if (HTREEITEM hItem = FindItem(item))
	{
		DWORD	tvisMask = NSTCITEMSTATE2TVIS(nstcisMask);
		DWORD	tvisFlags = NSTCITEMSTATE2TVIS(nstcisFlags);

		// SELECTED な項目が複数になってしまうので SelectItem で行う
		if (tvisMask & TVIS_SELECTED)
		{
			tvisMask &= ~TVIS_SELECTED;
			SelectItem(hItem);
		}

		// EXPANDED は効果が無いもよう
		if (tvisMask & TVIS_EXPANDED)
		{
			tvisMask &= ~TVIS_EXPANDED;
			if (tvisFlags & TVIS_EXPANDED)
				Expand(hItem);
			else
				Collapse(hItem);
		}

		SetItemState(hItem, tvisFlags, tvisMask);
		return S_OK;
	}
	return E_FAIL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetItemState(IShellItem* item, NSTCITEMSTATE nstcisMask, NSTCITEMSTATE *pnstcisFlags)
{
	if (HTREEITEM hItem = FindItem(item))
	{
		DWORD	tvisMask = NSTCITEMSTATE2TVIS(nstcisMask);
		DWORD	tvisFlags = GetItemState(hItem, tvisMask);
		*pnstcisFlags = TVIS2NSTCITEMSTATE(tvisFlags);
		return S_OK;
	}
	return E_FAIL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetSelectedItems(IShellItemArray** items)
{
	if (HTREEITEM hItem = GetSelection())
		if (IShellItem* item = GetItemData(hItem))
			return XpCreateShellItemArray(items, 1, &item);
	return E_FAIL;
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
	if (HTREEITEM hItem = FindItem(item))
	{
		EnsureVisible(hItem);
		return S_OK;
	}
	return E_FAIL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::SetTheme(PCWSTR pszTheme)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetNextItem(IShellItem* item, NSTCGNI nstcgi, IShellItem **pp)
{
	// TODO: nstcgi は TVGN と互換性があるか?
	if (HTREEITEM hItem = FindItem(item))
		if (HTREEITEM hNext = GetNextItem(hItem, nstcgi))
			return GetItemData(hNext, pp);
	return E_FAIL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::HitTest(POINT* pt, IShellItem** pp)
{
	TVHITTESTINFO hit = { pt->x, pt->y };
	if (HTREEITEM hItem = HitTest(&hit))
		return GetItemData(hItem, pp);
	return E_FAIL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::GetItemRect(IShellItem* item, RECT* rc)
{
	if (HTREEITEM hItem = FindItem(item))
		if (GetItemRect(hItem, rc))
			return S_OK;
	return E_FAIL;
}

IFACEMETHODIMP XpNameSpaceTreeControl::CollapseAll()
{
	for (HTREEITEM hItem = GetNextItem(TVI_ROOT, TVGN_CHILD);
		 hItem;
		 hItem = GetNextItem(hItem, TVGN_NEXT))
	{
		Collapse(hItem);
	}
	return S_OK;
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
