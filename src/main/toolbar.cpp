// toolbar.cpp
#include "stdafx.h"
#include "commctrl.hpp"
#include "sq.hpp"

const int MARGIN = 4;

static inline UINT DockToTPM(BYTE dock)
{
	switch (dock)
	{
	case CENTER:return TPM_CENTERALIGN | TPM_VCENTERALIGN;
	case SOUTH:	return TPM_LEFTALIGN   | TPM_BOTTOMALIGN;
	case WEST:	return TPM_RIGHTALIGN  | TPM_TOPALIGN;
	case EAST:	return TPM_LEFTALIGN   | TPM_TOPALIGN;
	default:	return TPM_LEFTALIGN   | TPM_TOPALIGN;
	}
}

//=============================================================================
#ifdef ENABLE_TOOLBAR

void ToolBar::constructor(sq::VM v)
{
	m_inDropDown = false;
	set_dock(NORTH);
	sq_resetobject(&m_items);
	__super::constructor(v);
}

void ToolBar::onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs)
{
	InitCommonControls(ICC_BAR_CLASSES);
	// WNDCLASSEX
	::GetClassInfoEx(NULL, TOOLBARCLASSNAME, &wc);
	wc.lpszClassName = L"ToolBar";
	// CREATESTRUCT
	cs.style |= TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP;
	cs.dwExStyle = 0;
}

LRESULT ToolBar::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MOUSEWHEEL:
		if (onMouseDown(msg, wParam, lParam))
			return 0;
		break;
	case WM_MOUSEMOVE:
		if (GET_KEYSTATE_WPARAM(wParam) & MK_BUTTONS)
		{
			POINT	pt = { GET_XY_LPARAM(lParam) };
			RECT	rc;
			int index = HitTest(pt.x, 0);
			if (index > 0 && GetItemRect(index, &rc) && pt.y >= rc.bottom)
			{
				onDropDown(index);
				return 0;
			}
		}
		break;
	case WM_SIZE:
		SetItemSize(GET_X_LPARAM(lParam) - MARGIN, -1);
		break;
	case TB_PRESSBUTTON:
		if (m_inDropDown)
			return 0;
		break;
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		SetCapture();
		return 0;
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		ReleaseCapture();
		onExecute(HitTest(GET_XY_LPARAM(lParam)), MSG2VK(msg, wParam), MSG2MK(msg, wParam));
		break;
	case WM_CONTEXTMENU:
	{
		POINT pt = { GET_XY_LPARAM(lParam) };
		ScreenToClient(&pt);
		if (HitTest(pt) >= 0)
			return 0;
		break;
	}
	case WM_CREATE:
		if (LRESULT lResult = __super::onMessage(msg, wParam, lParam))
			return lResult;
		DisableIME();
		SendMessage(CCM_SETVERSION, 6, 0);
		SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
		SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
		SendMessage(TB_SETINDENT, TOOLBAR_INDENT, 0);

		// ID = index になるよう index = 0 のダミーを挿入する。
		InsertItem(I_IMAGENONE, BTNS_SEP, TBSTATE_HIDDEN);
		AutoSize();
		return 0;
	case WM_DESTROY:
		sq_release(sq::vm, &m_items);
		break;
	}
	return __super::onMessage(msg, wParam, lParam);
}

LRESULT ToolBar::onNotify(Window* from, NMHDR& nm)
{
	switch (nm.code)
	{
	case WM_COMMAND:
		onExecute((int)nm.idFrom, 0, 0);
		return 0;
	case TBN_DROPDOWN:
		onDropDown(((NMTOOLBAR&)nm).iItem);
		return TBDDRET_DEFAULT;
	default:
		return __super::onNotify(from, nm);
	}
}

void ToolBar::onUpdate(RECT& bounds) throw()
{
	int		n = GetItemCount();
	RECT	rcItem;
	if (n > 1 && GetItemRect(n - 1, &rcItem))
		bounds.top = rcItem.bottom + MARGIN;
	__super::onUpdate(bounds);
}

bool ToolBar::onGesture(const MSG& msg)
{
	POINT pt = { GET_XY_LPARAM(msg.lParam) };
	if (msg.hwnd != m_hwnd)
		MapWindowPoints(msg.hwnd, m_hwnd, &pt, 1);
	if (HitTest(pt) >= 0)
		return false;
	return __super::onGesture(msg);
}

bool ToolBar::onMouseDown(UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT pt = { GET_XY_LPARAM(lParam) };
	if (msg == WM_MOUSEWHEEL)
		ScreenToClient(&pt);

	int index = HitTest(pt);
	if (index <= 0 || IsPressed(index))
	{
		// メニュー表示中。キャンセルさせる。
		return false;
	}
	else
		return onDropDown(index);
}

void ToolBar::onExecute(int index, UINT vk, UINT mk)
{
	if (index >= 1)
	{
		SetKeyState(vk, mk);
		onEvent(L"onExecute", index - 1);
		ResetKeyState(vk);
	}
}

bool ToolBar::onDropDown(int index)
{
	ReleaseCapture();
	SendMessage(TB_PRESSBUTTON, index, MAKELONG(true, 0));
	m_inDropDown = true;
	UpdateWindow(m_hwnd);
	bool r = onEvent(L"onDropDown", index - 1);
	m_inDropDown = false;
	PostMessage(TB_PRESSBUTTON, index, MAKELONG(false, 0));
	return r;
}

static DWORD KeyToDropEffect(DWORD keys, DWORD effect)
{
	// ALT or CTRL+SHIFT -- DROPEFFECT_LINK
	// CTRL              -- DROPEFFECT_COPY
	// SHIFT             -- DROPEFFECT_MOVE
	// no modifier       -- MOVE -> COPY -> LINK
	DWORD op;
	if (keys & MK_CONTROL)
	{
		if (keys & MK_SHIFT)	op = DROPEFFECT_LINK;
		else					op = DROPEFFECT_COPY;
	}
	else if (keys & MK_SHIFT)			op = DROPEFFECT_MOVE;
	else if (keys & MK_ALT)				op = DROPEFFECT_LINK;
	else if (effect & DROPEFFECT_MOVE)	op = DROPEFFECT_MOVE;
	else if (effect & DROPEFFECT_COPY)	op = DROPEFFECT_COPY;
	else								op = DROPEFFECT_LINK;

	return (op & effect);
}

static HRESULT PathDrop(IShellItem* item, IDataObject* data, DWORD keys, POINT ptScreen, DWORD* effect)
{
	HRESULT	hr;
	ref<IFileOperation>	op;
	ref<IDropTarget>	drop;
	SFGAOF				flags = 0;

	ASSERT(item);

	// フォルダへの DnD は処理してくれないようなので、
	// 自前で IFileOperation を利用した処理を行う。
	hr = E_FAIL;
	if (SUCCEEDED(item->GetAttributes(SFGAO_FILESYSTEM | SFGAO_FOLDER, &flags)) &&
		(flags & (SFGAO_FILESYSTEM | SFGAO_FOLDER)) == (SFGAO_FILESYSTEM | SFGAO_FOLDER))
	{
		hr = FileOperate(&op);
	}
	else
	{
		hr = item->BindToHandler(null, BHID_SFUIObject, IID_PPV_ARGS(&drop));
	}

	if (!op && !drop)
		return hr;

	if (keys & MK_RBUTTON)
	{
		// 右ドロップ時にキーが押されていないと「ショートカットを作成」がデフォルトになってしまうため。
		if ((keys & MK_KEYS) == 0)
			keys |= MK_SHIFT;
	}
	else
		*effect = KeyToDropEffect(keys, *effect);

	if (op)
	{
		switch (*effect)
		{
		case DROPEFFECT_COPY:
			hr = FileCopy(op, data, item);
			break;
		case DROPEFFECT_MOVE:
			hr = FileMove(op, data, item);
			break;
		case DROPEFFECT_LINK:
			hr = E_NOTIMPL;
			//LinkCreate();
			//hr = FileLink(op, data, item);
			break;
		default:
			hr = E_INVALIDARG;
			break;
		}
		if SUCCEEDED(hr)
			hr = op->PerformOperations();
	}
	else
	{
		hr = drop->Drop(data, keys, (POINTL&)ptScreen, effect);
	}
	return hr;
}

Window* ToolBar::onDragOver(DWORD keys, POINT ptScreen, DWORD* effect)
{
	POINT pt = ScreenToClient(ptScreen);
	int index = HitTest(pt);
	SetHotItem(index);
	if (index <= 0)
		return __super::onDragOver(keys, ptScreen, effect);
	*effect = KeyToDropEffect(keys, *effect);
	return this;
}

void ToolBar::onDragDrop(IDataObject* data, DWORD keys, POINT ptScreen, DWORD* effect)
{
	POINT pt = ScreenToClient(ptScreen);
	int index = HitTest(pt);
	if (index > 0)
	{
		if (sq::VM v = sq::vm)
		{
			SQInteger top = v.gettop();
			try
			{
				// m_items[index - 1].Drop()
				v.push(m_items);
				v.push(index - 1);
				if SQ_SUCCEEDED(sq_get(v, -2))
				{
					IShellItem* item;
					if (sq_gettype(v, -1) != OT_USERDATA)
						v.getslot(-1, L"inner");
					sq::get(v, -1, &item);
					PathDrop(item, data, keys, ptScreen, effect);
				}
			}
			catch (sq::Error&)
			{
				TRACE(L"Drop Failed");
			}
			sq_reseterror(v);
			v.settop(top);
		}

		SetHotItem(-1);
	}
	else
	{
		SetHotItem(-1);
		__super::onDragDrop(data, keys, ptScreen, effect);
	}
}

void ToolBar::onDragLeave()
{
	SetHotItem(-1);
	__super::onDragLeave();
}

IImageList* ToolBar::get_icons() const
{
	return m_icons;
}

void ToolBar::set_icons(IImageList* value)
{
	if (m_icons != value)
	{
		SendMessage(TB_SETIMAGELIST			, 0, (LPARAM)value);
		SendMessage(TB_SETIMAGELIST			, 1, (LPARAM)(value ? SHGetImageList(16) : null));
//		SendMessage(TB_SETDISABLEDIMAGELIST	, 0, (LPARAM)value->Handle[DISABLED]);
//		SendMessage(TB_SETHOTIMAGELIST		, 0, (LPARAM)value->Handle[HOT]);
	}
}

SQInteger ToolBar::get_items(sq::VM v)
{
	// FIXME: 変更できないプロキシを返却すべき。
	v.push(m_items);
	return 1;
}

static inline bool IsShellIcon(int index)
{
	return HIWORD(index) == 1;
}

SQInteger ToolBar::set_items(sq::VM v)
{
	SQInteger				res;
	std::vector<TBBUTTON>	buttons;
	std::vector<HSQOBJECT>	names;
	BYTE					horz;

	switch (get_dock())
	{
	case WEST:
	case EAST:
		horz = 0;
		break;
	default:
		horz = BTNS_AUTOSIZE;
		break;
	}

	try
	{
		bool	useShellIcon = false;

		foreach (v, 2)
		{
			SQInteger idx = v.gettop();
			TBBUTTON button = { 0 };

			if (PCWSTR name = item_name(v, idx))
			{
				HSQOBJECT	obj;
				sq::get(v, -1, &obj);
				sq_addref(v, &obj);
				names.push_back(obj);

				int check = item_check(v, idx);

				button.iBitmap = item_icon(v, idx);
				button.idCommand = (int)(buttons.size() + 1);
				button.iString = (INT_PTR)name;
				button.fsState = TBSTATE_ENABLED;
				if (check < 0)
				{
					button.fsStyle = BTNS_BUTTON | BTNS_NOPREFIX | horz;
				}
				else
				{
					button.fsStyle = BTNS_CHECK | BTNS_NOPREFIX | horz;
					if (check > 0)
						button.fsState |= TBSTATE_CHECKED;
				}

				if (IsShellIcon(button.iBitmap))
					useShellIcon = true;
			}
			else
			{
				button.fsState = BTNS_SEP;
			}
			buttons.push_back(button);
		}

		DeleteAllItems();
		sq_release(v, &m_items);
		sq::get(v, 2, &m_items);
		sq_addref(v, &m_items);
		if (!buttons.empty())
			InsertItems(buttons.size(), &buttons[0]);

		if (useShellIcon)
			SendMessage(TB_SETIMAGELIST, 1, (LPARAM)SHGetImageList(16));

		if (horz)
		{
			SetRows(1);
		}
		else
		{
			SIZE	sz = GetClientSize(m_hwnd);
			SetItemSize(sz.cx - MARGIN, -1);
			SetRows(buttons.size());
			set_width(sz.cx);
		}

		res = 0;
	}
	catch (sq::Error&)
	{
		DeleteAllItems();
		sq_release(v, &m_items);
		res = SQ_ERROR;
	}

	for (size_t i = 0; i < names.size(); ++i)
		sq_release(v, &names[i]);

	update();
	return res;
}

POINT ToolBar::menupos(int index) const
{
	RECT rc;
	if (GetItemRect(index + 1, &rc))
	{
		POINT pt = ClientToScreen(RECT_SW(rc));
		pt.y -= 1;	// テーマ依存だと思うが、多少ずらしたほうが綺麗。
		return pt;
	}
	else
		throw sq::Error(L"ToolBar.menupos() : out of range");
}

#endif
