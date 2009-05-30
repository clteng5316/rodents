// treeview.cpp
#include "stdafx.h"
#include "treeview.hpp"
#include "sq.hpp"
#include "resource.h"

#ifdef ENABLE_TREEVIEW

//==========================================================================================================================

namespace
{
	static HRESULT FolderTreeCreate(TreeView* owner, const RECT& bounds, INameSpaceTreeControl** ppControl, DWORD* ppCookie)
	{
		HRESULT	hr;
		ref<INameSpaceTreeControl>			control;
		ref<INameSpaceTreeControlEvents>	events;
		HWND hwnd = owner->Handle;
		DWORD cookie = 0;

		if FAILED(hr = control.newobj(CLSID_NamespaceTreeControl))
			return hr;

		if SUCCEEDED(hr = IUnknown_SetSite(control, static_cast<IServiceProvider*>(owner)))
		{
			if SUCCEEDED(hr = owner->QueryInterface(&events))
				control->TreeAdvise(events, &cookie);
			RECT rc = bounds;
			if SUCCEEDED(hr = control->Initialize(hwnd, &rc,
				NSTCS_HASEXPANDOS |
				NSTCS_SINGLECLICKEXPAND |
				NSTCS_SPRINGEXPAND |
				NSTCS_SHOWSELECTIONALWAYS))
			{
				*ppControl = control.detach();
				*ppCookie = cookie;
				return S_OK;
			}
			// on error
			if (events)
				control->TreeUnadvise(cookie);
			IUnknown_SetSite(control, null);
		}
		return hr;
	}

	static void FolderTreeDestroy(ref<INameSpaceTreeControl>& control, DWORD& cookie)
	{
		if (control)
		{
			ref<INameSpaceTreeControl> _control(control);
			DWORD _cookie = cookie;
			control = null;
			cookie = 0;

			_control->TreeUnadvise(_cookie);
			IUnknown_SetSite(_control, null);
		}
	}
}

//==========================================================================================================================

ref<IShellItem> TreeView::get_selection() const
{
	ref<IShellItemArray> items;
	if SUCCEEDED(m_control->GetSelectedItems(&items))
	{
		ref<IShellItem> item;
		if SUCCEEDED(items->GetItemAt(0, &item))
			return item;
	}
	return null;
}

void TreeView::set_selection(IShellItem* value)
{
	m_control->SetItemState(value, NSTCIS_SELECTED, NSTCIS_SELECTED);
	m_control->EnsureItemVisible(value);
}

void TreeView::collapse(IShellItem* value)
{
	if (value)
		m_control->SetItemState(value, NSTCIS_EXPANDED, 0);
	else
		m_control->CollapseAll();
}

void TreeView::only(IShellItem* value)
{
	LockRedraw lock(m_hwnd);
	m_control->CollapseAll();
	if (value)
		set_selection(value);
}

void TreeView::toggle(IShellItem* value)
{
	NSTCITEMSTATE	state;
	if SUCCEEDED(m_control->GetItemState(value, NSTCIS_EXPANDED, &state))
	{
		state ^= NSTCIS_EXPANDED;
		m_control->SetItemState(value, NSTCIS_EXPANDED, state);
		m_control->EnsureItemVisible(value);
	}
}

void TreeView::append(IShellItem* item, bool includeFiles)
{
	m_control->AppendRoot(item,
		SHCONTF_FOLDERS | (includeFiles ? SHCONTF_NONFOLDERS : 0),
		NSTCRS_VISIBLE | NSTCRS_EXPANDED, null);
}

//==========================================================================================================================

void TreeView::onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs)
{
	m_inExecute = false;
	// WNDCLASSEX
	wc.hCursor = ::LoadCursor(null, IDC_ARROW);
	wc.lpszClassName = L"TreeView";
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = ::GetModuleHandle(null);
	// CREATESTRUCT
	cs.dwExStyle = 0;
}

HWND TreeView::onFocus() throw()
{
	return m_treeview;
}

LRESULT TreeView::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		if (m_treeview.IsWindow())
			return 0;
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (m_treeview)
		{
			return m_treeview.SendMessage(msg, wParam, lParam);
		}
		break;
	case WM_VSCROLL:
	case WM_HSCROLL:
		return m_treeview.SendMessage(msg, wParam, lParam);
	case WM_CREATE:
		if (LRESULT lResult = __super::onMessage(msg, wParam, lParam))
			return lResult;
		FolderTreeCreate(this, get_CenterArea(), &m_control, &m_cookie);
		if (Hwnd hwnd = Hwnd::from(m_control))
		{
			m_treeview = ::FindWindowEx(hwnd, null, WC_TREEVIEW, null);
			if (!m_treeview)
			{
				// XpNameSpaceTreeControl の場合はそのもの
				m_treeview = hwnd;
			}
		}
		ASSERT(m_treeview);
		return 0;
	case WM_DESTROY:
		m_treeview = null;
		FolderTreeDestroy(m_control, m_cookie);
		break;
	case WM_SIZE:
		if (Hwnd hwnd = Hwnd::from(m_control))
			hwnd.set_bounds(get_CenterArea());
		break;
	default:
		if (m_treeview && TV_FIRST <= msg && msg <= TV_LAST)
			return m_treeview.SendMessageW(msg, wParam, lParam);
	}
	return __super::onMessage(msg, wParam, lParam);
}

//==========================================================================================================================
// IServiceProvider

IFACEMETHODIMP TreeView::QueryService(REFGUID guidService, REFIID riid, void **pp)
{
	*pp = null;
	return E_NOINTERFACE;
}

//==========================================================================================================================
// INameSpaceTreeControlEvents

IFACEMETHODIMP TreeView::OnItemClick(IShellItem* item, NSTCEHITTEST hit, NSTCECLICKTYPE click)
{
	// 中クリック時の無限再帰を避けるため。
	if (m_inExecute)
		return S_FALSE;

	if (hit & (NSTCEHT_ONITEMICON | NSTCEHT_ONITEMLABEL))
	{
		const UINT NSTCECT2MK[] = { 0, MK_LBUTTON, MK_MBUTTON, MK_RBUTTON };
		UINT mk = NSTCECT2MK[click & NSTCECT_BUTTON];
		m_inExecute = true;
		SetKeyState(0, mk | GetModifiers());
		onEvent(L"onExecute", item);
		ResetKeyState(0);
		m_inExecute = false;
	}

	return S_FALSE;
}

IFACEMETHODIMP TreeView::OnPropertyItemCommit(IShellItem* item)											{ return S_FALSE; }
IFACEMETHODIMP TreeView::OnItemStateChanging(IShellItem* item, NSTCITEMSTATE mask, NSTCITEMSTATE state)	{ return S_OK; }
IFACEMETHODIMP TreeView::OnItemStateChanged(IShellItem* item, NSTCITEMSTATE mask, NSTCITEMSTATE state)	{ return S_OK; }
IFACEMETHODIMP TreeView::OnSelectionChanged(IShellItemArray* items)										{ return S_OK; }
IFACEMETHODIMP TreeView::OnKeyboardInput(UINT msg, WPARAM wParam, LPARAM lParam)						{ return S_FALSE; }
IFACEMETHODIMP TreeView::OnBeforeExpand(IShellItem* item)												{ return S_OK; }
IFACEMETHODIMP TreeView::OnAfterExpand(IShellItem* item)												{ return S_OK; }
IFACEMETHODIMP TreeView::OnBeginLabelEdit(IShellItem* item)												{ return S_OK; }
IFACEMETHODIMP TreeView::OnEndLabelEdit(IShellItem* item)												{ return S_OK; }
IFACEMETHODIMP TreeView::OnGetToolTip(IShellItem* item, LPWSTR pszTip, int cchTip)						{ return E_NOTIMPL; }
IFACEMETHODIMP TreeView::OnBeforeItemDelete(IShellItem* item)											{ return E_NOTIMPL; }
IFACEMETHODIMP TreeView::OnItemAdded(IShellItem* item, BOOL root)										{ return E_NOTIMPL; }
IFACEMETHODIMP TreeView::OnItemDeleted(IShellItem* item, BOOL root)										{ return E_NOTIMPL; }
IFACEMETHODIMP TreeView::OnBeforeContextMenu(IShellItem* item, REFIID iid, void** pp)					{ *pp = null; return E_NOTIMPL; }
IFACEMETHODIMP TreeView::OnAfterContextMenu(IShellItem* item, IContextMenu* menu, REFIID iid, void** pp){ *pp = null; return E_NOTIMPL; }
IFACEMETHODIMP TreeView::OnBeforeStateImageChange(IShellItem* item)										{ return S_OK; }
IFACEMETHODIMP TreeView::OnGetDefaultIconIndex(IShellItem* item, int* defaultIcon, int* openIcon)		{ return E_NOTIMPL; }

#endif
