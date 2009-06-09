// xp_explorerbrowser.cpp

#include "stdafx.h"
#include "hwnd.hpp"
#include "unknown.hpp"
#include "math.hpp"

//==========================================================================================================================

namespace
{
	const size_t		MAX_HISTORY = 20;

	class History
	{
	private:
		std::vector<ITEMIDLIST*>	m_items;
		ssize_t						m_pos;
	public:
		History() : m_pos(0)	{}
		~History()
		{
			pop_back(0);
		}
		void push(const ITEMIDLIST* item) throw()
		{
			if (!m_items.empty())
				pop_back(m_pos + 1);
			if (m_items.size() > MAX_HISTORY)
				pop_front();
			m_pos = m_items.size();
			m_items.push_back(ILClone(item));
		}
		const ITEMIDLIST* operator [] (ssize_t diff) const throw()
		{
			ssize_t newpos = m_pos + diff;
			if (newpos < 0 || newpos >= m_items.size())
				return null;
			return m_items[newpos];
		}
		void go(ssize_t diff) throw()
		{
			if (!m_items.empty())
				m_pos = math::clamp<ssize_t>(m_pos + diff, 0, m_items.size() - 1);
		}
		void pop_back(size_t pos) throw()
		{
			for (size_t i = pos; i < m_items.size(); ++i)
				ILFree(m_items[i]);
			m_items.resize(pos);
		}
		void pop_front() throw()
		{
			ASSERT(!m_items.empty());
			m_items.erase(m_items.begin());
		}
	};

	class XpExplorerBrowser :
		public Unknown< implements<IExplorerBrowser, IObjectWithSite> >,
		public Hwnd
	{
	private:
		ref<IShellBrowser>			m_site;
		ref<IExplorerBrowserEvents>	m_handler;
		Hwnd						m_defview;
		EXPLORER_BROWSER_OPTIONS	m_options;
		ref<IShellView>				m_view;
		FOLDERSETTINGS				m_settings;
		RECT						m_bounds;
		History						m_history;

		const ITEMIDLIST* get_m_item() const throw()	{ return m_history[0]; }
		__declspec(property(get=get_m_item)) const ITEMIDLIST* m_item;

	public: // IExplorerBrowser
		IFACEMETHODIMP Initialize(HWND hwndParent, const RECT* rc, const FOLDERSETTINGS* fs);
		IFACEMETHODIMP Destroy();
		IFACEMETHODIMP SetRect(HDWP* phdwp, RECT rc);
		IFACEMETHODIMP SetPropertyBag(PCWSTR pszPropertyBag);
		IFACEMETHODIMP SetEmptyText(PCWSTR pszEmptryText);
		IFACEMETHODIMP SetFolderSettings(const FOLDERSETTINGS *pfs);
		IFACEMETHODIMP Advise(IExplorerBrowserEvents* handler, DWORD* cookie);
		IFACEMETHODIMP Unadvise(DWORD cookie);
		IFACEMETHODIMP SetOptions(EXPLORER_BROWSER_OPTIONS dwFlag);
		IFACEMETHODIMP GetOptions(EXPLORER_BROWSER_OPTIONS *pdwFlag);
		IFACEMETHODIMP BrowseToIDList(const ITEMIDLIST* pidl, UINT flags);
		IFACEMETHODIMP BrowseToObject(IUnknown *punk, UINT flags);
		IFACEMETHODIMP FillFromObject(IUnknown *punk, EXPLORER_BROWSER_FILL_FLAGS flags);
		IFACEMETHODIMP RemoveAll();
		IFACEMETHODIMP GetCurrentView(REFIID riid, void** pp);

	public: // IObjectWithSite
		IFACEMETHODIMP SetSite(IUnknown* site);
	    IFACEMETHODIMP GetSite(REFIID iid, void** pp);

	private:
		void	InvokeViewCreated(IShellView* view) const;
		void	InvokeNavigationComplete(const ITEMIDLIST* folder) const;
		void	InvokeNavigationFailed(const ITEMIDLIST* folder) const;
		HRESULT GoAbsolute(const ITEMIDLIST* item, ssize_t diff) throw();
		HRESULT GoRelative(const ITEMIDLIST* relative) throw();
		HRESULT GoUp() throw();
		HRESULT GoBack() throw();
		HRESULT GoForward() throw();
	};
}

inline void XpExplorerBrowser::InvokeViewCreated(IShellView* view) const
{
	if (m_handler)
		m_handler->OnViewCreated(view);
}

inline void XpExplorerBrowser::InvokeNavigationComplete(const ITEMIDLIST* folder) const
{
	if (m_handler)
		m_handler->OnNavigationComplete(folder);
}

inline void XpExplorerBrowser::InvokeNavigationFailed(const ITEMIDLIST* folder) const
{
	if (m_handler)
		m_handler->OnNavigationFailed(folder);
}

//==========================================================================================================================
// IExplorerBrowser

IFACEMETHODIMP XpExplorerBrowser::Initialize(HWND hwndParent, const RECT* rc, const FOLDERSETTINGS* fs)
{
	ASSERT(rc);
	ASSERT(fs);
	m_hwnd = hwndParent;
	m_bounds = *rc;
	m_settings = *fs;
	return S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::Destroy()
{
	if (m_view)
	{
		m_view->SaveViewState();
		m_view->DestroyViewWindow();
	}
	m_site = null;
	m_handler = null;
	m_defview = null;
	return S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::SetRect(HDWP* phdwp, RECT rc)
{
	m_bounds = rc;
	if (m_defview)
	{
		if (phdwp)
			*phdwp = ::DeferWindowPos(*phdwp, m_defview, null, RECT_XYWH(rc), SWP_NOZORDER | SWP_NOACTIVATE);
		else
			m_defview.set_bounds(rc);
	}
	return S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::SetPropertyBag(PCWSTR pszPropertyBag)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpExplorerBrowser::SetEmptyText(PCWSTR pszEmptryText)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpExplorerBrowser::SetFolderSettings(const FOLDERSETTINGS *pfs)
{
	HRESULT	hr;

	if (!pfs)
		return E_POINTER;

	ref<IFolderView> view;
	if FAILED(GetCurrentView(IID_PPV_ARGS(&view)))
		return E_FAIL;

	if FAILED(hr = view->SetCurrentViewMode(pfs->ViewMode))
		return hr;
	// TODO: update pfs->fFlags, maybe requires re-create view.
	m_settings = *pfs;
	return S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::Advise(IExplorerBrowserEvents* handler, DWORD* pdwCookie)
{
	if (m_handler)
		return E_FAIL;
	m_handler = handler;
	if (pdwCookie)
		*pdwCookie = 0xDEADBEEF;
	return S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::Unadvise(DWORD dwCookie)
{
	if (dwCookie != 0xDEADBEEF)
		return E_INVALIDARG;
	m_handler = null;
	return S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::SetOptions(EXPLORER_BROWSER_OPTIONS dwFlag)
{
	m_options = dwFlag;
	return S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::GetOptions(EXPLORER_BROWSER_OPTIONS *pdwFlag)
{
	*pdwFlag = m_options;
	return S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::BrowseToIDList(const ITEMIDLIST* pidl, UINT flags)
{
	if (flags & SBSP_NAVIGATEBACK)
		return GoBack();
	else if (flags & SBSP_NAVIGATEFORWARD)
		return GoForward();
	else if (flags & SBSP_PARENT)
		return GoUp();
	else if (flags & SBSP_RELATIVE)
		return GoRelative(pidl);
	else
		return GoAbsolute(pidl, 0);
}

IFACEMETHODIMP XpExplorerBrowser::BrowseToObject(IUnknown *punk, UINT flags)
{
	if (flags & SBSP_NAVIGATEBACK)
		return GoBack();
	else if (flags & SBSP_NAVIGATEFORWARD)
		return GoForward();
	else if (flags & SBSP_PARENT)
		return GoUp();
	else if (ILPtr item = ILCreate(punk))
		return GoAbsolute(item, 0);
	else
		return E_NOTIMPL;
}

IFACEMETHODIMP XpExplorerBrowser::FillFromObject(IUnknown *punk, EXPLORER_BROWSER_FILL_FLAGS flags)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpExplorerBrowser::RemoveAll()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpExplorerBrowser::GetCurrentView(REFIID riid, void** pp)
{
	if (!m_view)
	{
		*pp = null;
		return E_UNEXPECTED;
	}
	return m_view->QueryInterface(riid, pp);
}

//==========================================================================================================================

IFACEMETHODIMP XpExplorerBrowser::SetSite(IUnknown* site)
{
	m_site = null;
	return site ? site->QueryInterface(&m_site) : S_OK;
}

IFACEMETHODIMP XpExplorerBrowser::GetSite(REFIID iid, void** pp)
{
	return m_site.copyto(iid, pp);
}

//==========================================================================================================================

HRESULT XpExplorerBrowser::GoAbsolute(const ITEMIDLIST* item, ssize_t diff) throw()
{
	if (!item)
		return E_INVALIDARG;

	if (!m_site || !IsWindow())
		return E_UNEXPECTED;

	ILPtr focus; // 子→親へ移動した場合は、それをフォーカスする。

	if (m_item)
	{
		if (ILIsEqual(m_item, item))
			return S_FALSE; // 移動の必要なし
		if (ILIsParent(item, m_item, FALSE))
		{
			size_t size = ILGetSize(item);
			focus.attach(ILCloneFirst((ITEMIDLIST*)(((BYTE*)m_item) + size - 2)));
		}
	}

	HRESULT				hr;
	ref<IShellFolder>	folder;
	ref<IShellView>		view;
	HWND				hwndDefView = null;

	if (FAILED(hr = ILFolder(&folder, item)) ||
		FAILED(hr = folder->CreateViewObject(m_hwnd, IID_PPV_ARGS(&view))))
		return hr;

	bool hasFocus = HasFocus(m_defview);

	// 現在の情報を取得、保存しておく。
	if (m_defview)
		m_bounds = m_defview.get_bounds();
	if (m_view)
	{
		m_view->GetCurrentInfo(&m_settings);
		m_view->SaveViewState();
	}

	// 移動する。
	// 本来は m_site->QueryService() すべきなのだが、
	// 現実装では m_site->QueryInterface() で十分であるため、直接渡している。
	if FAILED(hr = view->CreateViewWindow(m_view, &m_settings, m_site, &m_bounds, &hwndDefView))
		return hr;

	// 作成に成功したのでメンバを更新
	m_view = view;
	m_defview = hwndDefView;
	if (diff == 0)
	{
		m_history.push(item);
		ASSERT(ILEquals(item, m_item));	/* check equivalence */
	}
	else
	{
		m_history.go(diff);
		ASSERT(item == m_item);	/* check identity */
	}

	// コールバック。
	InvokeViewCreated(m_view);

	m_view->UIActivate(hasFocus ? SVUIA_ACTIVATE_FOCUS : SVUIA_ACTIVATE_NOFOCUS);

	if (HWND hwndListView = ::FindWindowEx(m_defview, null, WC_LISTVIEW, null))
	{
		// 親への移動は、さっきまでいた子を選択状態にする。さもなければフォーカス状態のアイテムを選択する。
		// ListViewH のアイテム数がゼロの場合、まだアイテムの取得が完了していない可能性がある。
		if (ListView_GetItemCount(hwndListView) > 0 && focus)
			m_view->SelectItem(focus, SVSI_FOCUSED | SVSI_SELECT | SVSI_ENSUREVISIBLE);
	}

	InvokeNavigationComplete(item);

	return S_OK;
}

HRESULT XpExplorerBrowser::GoRelative(const ITEMIDLIST* relative) throw()
{
	if (!relative)
		return E_INVALIDARG;
	if (const ITEMIDLIST* item = m_item)
		return GoAbsolute(ILCombine(item, relative), 0);
	else
		return E_UNEXPECTED;
}

HRESULT XpExplorerBrowser::GoUp() throw()
{
	ILPtr parent = ILCloneParent(m_item);
	if (!parent)
		return S_FALSE;
	return GoAbsolute(parent, 0);
}

HRESULT XpExplorerBrowser::GoBack() throw()
{
	if (const ITEMIDLIST* item = m_history[-1])
		return GoAbsolute(item, -1);
	return S_FALSE;
}

HRESULT XpExplorerBrowser::GoForward() throw()
{
	if (const ITEMIDLIST* item = m_history[+1])
		return GoAbsolute(item, +1);
	return S_FALSE;
}

//==========================================================================================================================

HRESULT XpExplorerBrowserCreate(REFINTF pp)
{
	return newobj<XpExplorerBrowser>(pp);
}
