// dialog.cpp
#include "stdafx.h"
#include "commctrl.hpp"
#include "sq.hpp"
#include "math.hpp"
#include "string.hpp"
#include "resource.h"

//=============================================================================

void Dialog::onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs)
{
	// WNDCLASSEX
	wc.hCursor = ::LoadCursor(null, IDC_ARROW);
	wc.lpszClassName = L"Dialog";
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = ::GetModuleHandle(null);
	// CREATESTRUCT
	cs.style = DS_MODALFRAME | WS_POPUP | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU;
	cs.dwExStyle = WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT;
	if (cs.hwndParent)
		cs.dwExStyle |= WS_EX_TOOLWINDOW;

	m_owner = GetAncestor(cs.hwndParent, GA_ROOT);
}

LRESULT	Dialog::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SETFOCUS:
		::SetFocus(::GetWindow(m_hwnd, GW_CHILD));
		return 0;
	case WM_ENDSESSION:
		if (!wParam)
			break;
		// fall through
	case WM_CLOSE:
		if (m_owner)
			::EnableWindow(::GetAncestor(m_owner, GA_ROOT), true);
		break;
	case WM_CREATE:
		if (LRESULT lResult = __super::onMessage(msg, wParam, lParam))
			return lResult;
		if (HMENU sysmenu = ::GetSystemMenu(m_hwnd, false))
		{
			::RemoveMenu(sysmenu, SC_MAXIMIZE, MF_BYCOMMAND);
			::RemoveMenu(sysmenu, SC_MINIMIZE, MF_BYCOMMAND);
			::RemoveMenu(sysmenu, SC_RESTORE, MF_BYCOMMAND);
		}
		return 0;
	}
	return __super::onMessage(msg, wParam, lParam);
}

bool Dialog::onPress(const MSG& msg) throw()
{
	if (msg.message == WM_KEYDOWN &&
		(msg.hwnd == m_hwnd || ::GetParent(msg.hwnd) == m_hwnd))
	{
		switch (msg.wParam)
		{
		case VK_ESCAPE:
			onClose(IDCANCEL);
			return true;
		case VK_TAB:
			if (msg.hwnd == m_hwnd)
				::SetFocus(::GetWindow(msg.hwnd, GW_CHILD));
			else if (HWND next = ::GetWindow(msg.hwnd, GW_HWNDNEXT))
				::SetFocus(next);
			else
				::SetFocus(::GetWindow(msg.hwnd, GW_HWNDFIRST));
			return true;
		}
	}
	return __super::onPress(msg);
}

bool Dialog::onGesture(const MSG& msg)
{
	return false;
}

void Dialog::onClose(int id)
{
	close();
}

void Dialog::run()
{
	if (!IsWindow())
		throw sq::Error(L"Dialog already disposed");

	if (m_owner)
	{
		// この状態ではまだ非表示
		CenterWindow(m_hwnd, m_owner);
		if (m_owner)
			::EnableWindow(m_owner, false);
		set_visible(true);
		SetFocus();

		while (IsWindow())
		{
			PumpMessage();
		}

		if (m_owner)
		{	// WM_CLOSEで処理済のはずだが、念のため。
			::EnableWindow(m_owner, true);
			::SetActiveWindow(m_owner);
			::SetFocus(m_owner);
		}
	}
	else
		__super::run();
}

void Dialog::setTip(HWND hwnd, UINT message)
{
	if (!m_tip)
		m_tip.Create(m_hwnd);

	RECT	rc;
	::GetWindowRect(hwnd, &rc);
	m_tip.Add(m_hwnd, hwnd, _S(message), rc);
}

//=============================================================================

static const int	DLG_MARGIN			= 8;
static const int	DLG_BUTTON_WIDTH	= 80;
static const int	DLG_BUTTON_HEIGHT	= 24;
static const int	DLG_FORMAT_WIDTH	= 200;

static const int	RENAMEDLG_NEW	= 0;
static const int	RENAMEDLG_OLD	= 1;

static void EditLabel(ListViewH lv)
{
	int index = lv.GetNextItem(-1, LVNI_SELECTED);
	if (index >= 0)
		ListView_EditLabel(lv, index);
}

static void BeginLabelEdit(ListViewH lv, int index)
{
	if (EditH edit = lv.GetEdit())
	{
		WCHAR	name[MAX_PATH];
//		ref<IShellFolder> folder;
//		if SUCCEEDED(m_entry->QueryObject(&folder))
//			::SHLimitInputEdit(hEdit, folder);
		SubclassSingleLineEdit(sq::vm, edit);
		edit.GetText(name, lengthof(name));
		PCWSTR	ext = PathFindExtension(name);
		if (*ext == L'.' && ext > name)
			edit.SetSelection(0, (int)(ext - name));
	}
}

static bool EndLabelEdit(ListViewH lv, NMLVDISPINFOW* nm)
{
	if (!nm->item.pszText)
		return false;	// canceled

	int		index = nm->item.iItem;
	if (index + 1 < lv.GetItemCount())
	{	// 次のアイテムを編集
		::PostMessage(lv, LVM_EDITLABEL, index + 1, 0);
	}
	else
	{	// OKボタンにフォーカスを移す
		::PostMessage(GetParent(lv), WM_KEYDOWN, VK_TAB, 0);
	}
	return true;
}

static void RenameDialog_format(ListViewH lv, PCWSTR format)
{
	// parse "prefix <NNN> suffix" format.
	ssize_t	index, width = 1;
	ssize_t	beg, end;
	if (PCWSTR bra  = wcschr(format, L'<'))
	{
		PCWSTR num = bra + 1;
		while (*num != '\0' && !wcschr(L"0123456789>", *num)) { ++num; }
		index = _wtoi(num);
		beg = bra - format;
		if (PCWSTR cket = wcschr(bra, L'>'))
		{
			end = cket - format + 1;
			width = end - beg - 2;
		}
		else
			end = wcslen(format);
	}
	else
	{
		index = 1; // default: 1 base index
		beg = end = wcslen(format);
	}

	int	count = lv.GetItemCount();
	int dec = 0;
	for (size_t cnt = count; cnt > 0 && dec < 9; cnt /= 10) { ++dec; }
	WCHAR	fmt[] = L"%00d%s%s";
	fmt[2] = (WCHAR)(L'0' + math::clamp(width, dec, 9));

	for (int i = 0; i < count; ++i)
	{
		WCHAR	src[MAX_PATH];
		WCHAR	dst[MAX_PATH];

		lv.GetItemText(i, RENAMEDLG_NEW, src, lengthof(src));
		wcsncpy_s(dst, format, beg);
		swprintf_s(dst + beg, lengthof(dst) - beg, fmt,
			index + i, format + end, PathFindExtension(src));
		lv.SetItemText(i, RENAMEDLG_NEW, dst);
	}
}

LRESULT	RenameDialog::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SETFOCUS:
		if (m_listview)
		{
			::SetFocus(m_listview);
			return 0;
		}
		break;
	case WM_COMMAND:
		if ((HWND)lParam == m_ok)
		{
			onClose(IDOK);
			return 0;
		}
		else if ((HWND)lParam == m_cancel)
		{
			onClose(IDCANCEL);
			return 0;
		}
		break;
	case WM_NOTIFY:
	{
		NMHDR* nm = (NMHDR*)lParam;
		if (nm->hwndFrom == m_listview)
		{
			switch (nm->code)
			{
			case NM_CLICK:
				ListView_EditLabel(nm->hwndFrom, ((NMITEMACTIVATE*)nm)->iItem);
				return 0;
			case LVN_KEYDOWN:
				switch (((NMLVKEYDOWN*)nm)->wVKey)
				{
				case VK_RETURN:
				case VK_SPACE:
				case VK_F2:
					EditLabel(m_listview);
					return 0;
				}
				break;
			case LVN_BEGINLABELEDITW:
				BeginLabelEdit(m_listview, ((NMLVDISPINFOW*)lParam)->item.iItem);
				return 0;
			case LVN_ENDLABELEDITW:
				return EndLabelEdit(m_listview, (NMLVDISPINFOW*)nm);
			default:
				break;
			}
		}
		break;
	}
	case WM_CONTEXTMENU:
		if ((HWND)wParam == m_listview)
		{
			int		index;
			POINT	pt = { GET_XY_LPARAM(lParam) };
			if (pt.x == -1 && pt.y == -1)
			{
				index = m_listview.GetNextItem(-1, LVNI_FOCUSED);
				if (index >= 0)
				{
					RECT rc;
					m_listview.GetItemRect(index, &rc, LVIR_BOUNDS);
					pt = RECT_SW(rc);
					m_listview.ClientToScreen(&pt);
				}
			}
			else
			{
				POINT ptClient = m_listview.ScreenToClient(pt);
				index = m_listview.HitTest(POINT_XY(ptClient));
			}
			if (index >= 0)
			{
				MenuH popup = CreatePopupMenu();

				enum
				{
					ID_REVERT	= 1000,
					ID_EXCLUDE,
				};

				AppendMenu(popup, MF_STRING, ID_REVERT, _S(STR_REVERT));
				AppendMenu(popup, MF_STRING, ID_EXCLUDE, _S(STR_EXCLUDE));

				UINT cmd = ::TrackPopupMenu(popup, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, m_hwnd, 0);
				switch (cmd)
				{
				case ID_REVERT:
					revert(index);
					break;
				case ID_EXCLUDE:
					exclude(index);
					break;
				}
			}
			return 0;
		}
		break;
	case WM_CREATE:
		m_renamed = false;
		if (LRESULT lResult = __super::onMessage(msg, wParam, lParam))
			return lResult;
		onDialogCreate();
		return 0;
	case WM_SHOWWINDOW:
		if (!m_renamed && (BOOL)wParam)
		{
			m_renamed = true;
			// リネーム状態から開始する
			m_listview.PostMessage(LVM_EDITLABEL, 0, 0);
		}
		break;
	case WM_SIZE:
		onDialogResize(GET_XY_LPARAM(lParam));
		break;
	}

	return __super::onMessage(msg, wParam, lParam);
}

bool RenameDialog::onPress(const MSG& msg)
{
	if (msg.message == WM_KEYDOWN && msg.hwnd == m_format)
	{
		if (msg.wParam == VK_RETURN)
		{
			WCHAR	format[MAX_PATH];
			if (GetWindowText(m_format, format, lengthof(format)) > 0)
				RenameDialog_format(m_listview, format);
			return true;
		}
	}
	return __super::onPress(msg);
}

static void InsertColumn(ListViewH lv, int col, int order, PCWSTR value)
{
	LVCOLUMN	column = { LVCF_TEXT | LVCF_ORDER };
	column.pszText = (PWSTR)value;
	column.iSubItem = col;
	column.iOrder = order;
	lv.InsertColumn(column);
}

void RenameDialog::onDialogCreate()
{
	// Buttons
	m_ok.Create(m_hwnd, IDOK, _S(STR_OK));
	m_cancel.Create(m_hwnd, IDCANCEL, _S(STR_CANCEL));
	// Edit
	m_format.Create(m_hwnd, ES_AUTOHSCROLL);
	SubclassSingleLineEdit(sq::vm, m_format);
	::SetWindowText(m_format, L"<1>");
	setTip(m_format, STR_RENAME_EDIT);
//	setTip(m_format, "拡張子は保存されます。シフトキーを押しながらの場合は拡張子も置き換えられます。");
	// ListView
	m_listview.Create(m_hwnd, LVS_REPORT | LVS_EDITLABELS | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS);
	InsertColumn(m_listview, RENAMEDLG_NEW, 1, _S(STR_BEFORE));
	InsertColumn(m_listview, RENAMEDLG_OLD, 0, _S(STR_AFTER));
	m_listview.SetExtendedStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
}

void RenameDialog::onDialogResize(int w, int h)
{
	if (w <= 0 || h <= 0)
		return;

	RECT	r;

	SetRect(&r,
		DLG_MARGIN,
		DLG_MARGIN,
		w - DLG_MARGIN,
		h - DLG_MARGIN * 2 - DLG_BUTTON_HEIGHT);
	m_listview.set_bounds(r);

	SetRect(&r,
		DLG_MARGIN,
		h - DLG_MARGIN - DLG_BUTTON_HEIGHT,
		DLG_MARGIN + DLG_BUTTON_WIDTH,
		h - DLG_MARGIN);
	m_ok.set_bounds(r);

	SetRect(&r,
		DLG_MARGIN * 2 + DLG_BUTTON_WIDTH,
		h - DLG_MARGIN - DLG_BUTTON_HEIGHT,
		DLG_MARGIN * 2 + DLG_BUTTON_WIDTH * 2,
		h - DLG_MARGIN);
	m_cancel.set_bounds(r);

	SetRect(&r,
		DLG_MARGIN * 3 + DLG_BUTTON_WIDTH * 2,
		h - DLG_MARGIN - DLG_BUTTON_HEIGHT,
		w - DLG_MARGIN,
		h - DLG_MARGIN);
	if (RECT_W(r) > DLG_FORMAT_WIDTH)
		r.left = r.right - DLG_FORMAT_WIDTH;
	m_format.set_bounds(r);

	int	n = m_listview.GetColumnCount();
	LVCOLUMN column = { LVCF_WIDTH };
	column.cx = (w - DLG_MARGIN * 3 - ::GetSystemMetrics(SM_CXVSCROLL)) / n;	// 若干狭くする
	for (column.iSubItem = 0; column.iSubItem < n; column.iSubItem++)
		m_listview.SetColumn(column);
}

void RenameDialog::onClose(int id)
{
	if (id == IDOK && !m_items.empty())
	{
		int	n = m_listview.GetItemCount();
		for (int i = 0; i < n; i++)
		{
			WCHAR	src[MAX_PATH];
			WCHAR	dst[MAX_PATH];
			m_listview.GetItemText(i, RENAMEDLG_OLD, src, lengthof(src));
			m_listview.GetItemText(i, RENAMEDLG_NEW, dst, lengthof(dst));
			if (wcscmp(src, dst) != 0)
			{
				if (!m_op && FAILED(FileOperate(&m_op)))
					break;
				FileRename(m_op, m_items[i], dst);
			}
		}
	}
	__super::onClose(id);
}

void RenameDialog::run()
{
	__super::run();
	if (m_op)
	{
		m_op->PerformOperations();
		m_op = null;
	}
	m_items.clear();
}

SQInteger RenameDialog::set_items(sq::VM v)
{
	m_items.clear();
	m_listview.DeleteAllItems();

	LVITEM item = { LVIF_TEXT };

	try
	{
		foreach (v, 2)
		{
			IShellItem* i = v.get<IShellItem*>(-1);
			if (!i)
				continue;

			CoStr	name;
			if FAILED(i->GetDisplayName(SIGDN_PARENTRELATIVEEDITING, &name))
				continue;

			item.pszText = name;
			item.iSubItem = RENAMEDLG_NEW;
			m_listview.InsertItem(item);
			item.iSubItem = RENAMEDLG_OLD;
			m_listview.SetItem(item);
			item.iItem++;

			m_items.push_back(ref<IShellItem>(i));
		}
	}
	catch (sq::Error&)
	{
		m_items.clear();
		m_listview.DeleteAllItems();
		throw;
	}

	m_renamed = false;
	return 0;
}

SQInteger RenameDialog::set_names(sq::VM v)
{
	WCHAR	name[MAX_PATH];

	if (sq_gettype(v, 2) == OT_STRING)
	{
		str::copy_trim(name, lengthof(name), v.get<PCWSTR>(2));
		RenameDialog_format(m_listview, name);
	}
	else
	{
		int		count = m_listview.GetItemCount();
		int		i = 0;

		// それ以外は配列として扱う。
		foreach (v, 2)
		{
			v.tostring(-1);
			str::copy_trim(name, lengthof(name), v.get<PCWSTR>(-1));
			m_listview.SetItemText(i, RENAMEDLG_NEW, name);
			if (++i > count)
				break;
		}
	}
	m_renamed = true;
	return 0;
}

void RenameDialog::revert(int index)
{
	if ((size_t) index < m_items.size())
	{
		WCHAR	name[MAX_PATH];
		m_listview.GetItemText(index, 1, name, lengthof(name));
		m_listview.SetItemText(index, 0, name);
	}
}

void RenameDialog::exclude(int index)
{
	if ((size_t) index < m_items.size())
	{
		m_listview.DeleteItem(index);
		m_items.erase(m_items.begin() + index);
	}
}
