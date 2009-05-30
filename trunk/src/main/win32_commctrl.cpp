// win32_commctrl.cpp

#include "stdafx.h"
#include "win32_commctrl.hpp"
#include "math.hpp"
#include "ref.hpp"

//================================================================================

HWND Hwnd::from(IUnknown* obj) throw()
{
	HWND			hwnd;
	ref<IOleWindow>	w;
	if (obj &&
		SUCCEEDED(obj->QueryInterface(&w)) &&
		SUCCEEDED(w->GetWindow(&hwnd)))
		return hwnd;
	return null;
}

HWND Hwnd::from(IOleWindow* obj) throw()
{
	HWND	hwnd;
	if (obj && SUCCEEDED(obj->GetWindow(&hwnd)))
		return hwnd;
	return null;
}

HWND Hwnd::get_parent() const throw()
{
	HWND hwnd = ::GetAncestor(m_hwnd, GA_PARENT);
	if (hwnd && hwnd == ::GetDesktopWindow())
		return null;
	return hwnd;
}

bool Hwnd::get_visible() const throw()
{
	bool value = GetVisible(m_hwnd);
	if (!value || GetParent(m_hwnd))
		return value;
	return !IsIconic(m_hwnd) && GetActiveWindow() == m_hwnd;
}

static void ShowWindowNoAnimation(HWND hwnd, UINT what)
{
	ANIMATIONINFO info = { sizeof(ANIMATIONINFO) };
	bool	minAnimate = SystemParametersInfo(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &info, 0) && info.iMinAnimate;
	if (minAnimate)
	{
		info.iMinAnimate = false;
		SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &info, 0);
	}
	::ShowWindow(hwnd, what);
	if (minAnimate)
	{
		info.iMinAnimate = true;
		SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &info, 0);
	}
}

void Hwnd::set_visible(bool value) throw()
{
	if (m_hwnd != GetWindow())
		::ShowWindow(m_hwnd, value ? SW_RESTORE : SW_HIDE);
	else if (value)
	{
		if (IsIconic(m_hwnd))
			ShowWindowNoAnimation(m_hwnd, SW_RESTORE);
		// とりあえず全部呼んでおく
		BringWindowToTop(m_hwnd);
		SetForegroundWindow(m_hwnd);
		SetActiveWindow(m_hwnd);
	}
	else
		ShowWindowNoAnimation(m_hwnd, SW_MINIMIZE);
}

RECT Hwnd::get_bounds() const
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	if (HWND parent = get_parent())
		::ScreenToClient(parent, &rc);
	return rc;
}

POINT Hwnd::get_location() const throw()
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	if (HWND parent = get_parent())
		::ScreenToClient(parent, (POINT*)&rc);
	return (POINT&)rc;
}

void Hwnd::move(int x, int y) throw()
{
	ASSERT(IsWindow());
	::SetWindowPos(m_hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

SIZE Hwnd::get_size() const throw()
{
	ASSERT(IsWindow());
	RECT r;
	::GetWindowRect(m_hwnd, &r);
	return RECT_SIZE(r);
}

void Hwnd::resize(int w, int h) throw()
{
	ASSERT(IsWindow());
	HWND parent = get_parent();

	// ReBar の子供になっているとサイズが変更できなくなってしまうため。
	int count = ReBar_GetBandCount(parent);
	if (count > 0)
	{
		int index = ReBar_FindBand(parent, m_hwnd);
		if (0 <= index && index < count)
		{
			ReBar_Band band(RBBIM_CHILDSIZE);
			band.cxMinChild = w;
			band.cyMinChild = h;
			ReBar_SetBandInfo(parent, index, &band);
		}
	}

	::SetWindowPos(m_hwnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}

static void ShowWindowIf(HWND hwnd, DWORD ws, DWORD sw)
{
	for (; hwnd; hwnd = ::GetAncestor(hwnd, GA_PARENT))
	{
		DWORD	style = ::GetStyle(hwnd);
		if ((style & WS_VISIBLE) == 0)
			break;
		else if (style & ws)
		{
			::ShowWindow(hwnd, sw);
			break;
		}
	}
}

void Hwnd::maximize() throw()
{
	ShowWindowIf(m_hwnd, WS_MAXIMIZEBOX, SW_MAXIMIZE);
}

void Hwnd::minimize() throw()
{
	ShowWindowIf(m_hwnd, WS_MINIMIZEBOX, SW_MINIMIZE);
}

void Hwnd::restore() throw()
{
	ShowWindowIf(m_hwnd, WS_MAXIMIZEBOX | WS_MINIMIZEBOX, SW_RESTORE);
}

void Hwnd::set_width(int value) throw()
{
	resize(value, get_height());
}

void Hwnd::set_height(int value) throw()
{
	resize(get_width(), value);
}

void Hwnd::set_bounds(const RECT& value) throw()
{
	ASSERT(IsWindow());
	VERIFY(::SetWindowPos(m_hwnd, NULL, RECT_XYWH(value), SWP_NOZORDER | SWP_NOACTIVATE));
}

void Hwnd::ModifyStyle(DWORD dwRemove, DWORD dwAdd, DWORD dwRemoveEx, DWORD dwAddEx) throw()
{
	DWORD	dwStyle = GetStyle(m_hwnd);
	DWORD	dwStyleEx = GetStyleEx(m_hwnd);
	DWORD	dwStyle2 = (dwStyle & ~dwRemove | dwAdd);
	DWORD	dwStyleEx2 = (dwStyleEx & ~dwRemoveEx | dwAddEx);
	if (dwStyle2 != dwStyle)
		::SetWindowLong(m_hwnd, GWL_STYLE, dwStyle2);
	if (dwStyleEx2 != dwStyleEx)
		::SetWindowLong(m_hwnd, GWL_EXSTYLE, dwStyleEx2);
	if (dwStyle2 != dwStyle || dwStyleEx2 != dwStyleEx)
		::SetWindowPos(m_hwnd, null, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Hwnd::TrackMouseEvent(DWORD tme, DWORD time) throw()
{
	TRACKMOUSEEVENT e = { sizeof(TRACKMOUSEEVENT), tme, m_hwnd, time };
	::TrackMouseEvent(&e);
}

void Hwnd::SetLayerBitmap(HBITMAP bitmap, BYTE alpha) throw()
{
	BITMAP info;
	GetObject(bitmap, sizeof(BITMAP), &info);
	ModifyStyle(0, 0, 0, WS_EX_LAYERED);
	HDC dcScreen = ::GetDC(null);
	HDC dc = ::CreateCompatibleDC(dcScreen);
	HBITMAP old = SelectBitmap(dc, bitmap);
	POINT zero = { 0, 0 };
	SIZE size = { info.bmWidth, info.bmHeight };
	BLENDFUNCTION blend = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };
	::UpdateLayeredWindow(m_hwnd, dcScreen, NULL, &size, dc, &zero, 0, &blend, ULW_ALPHA);
	::SelectBitmap(dc, old);
	::DeleteDC(dc);
	::ReleaseDC(null, dcScreen);
}

//================================================================================
// Button

HWND Button_Create(HWND parent, UINT nID, PCWSTR title)
{
	HWND hwnd = ::CreateWindowEx(0, WC_BUTTON, title, WS_CHILD | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, (HMENU)(UINT_PTR)nID, ::GetModuleHandle(null), null);
	::SendMessage(hwnd, WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), 0);
	return hwnd;
}

//================================================================================
// Button

HWND Edit_Create(HWND parent, DWORD style)
{
	HWND hwnd = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, null, WS_CHILD | WS_VISIBLE | style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, null, ::GetModuleHandle(null), null);
	::SendMessage(hwnd, WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), 0);
	return hwnd;
}

//================================================================================

static void Header_GetColumnWidth(HWND header, HDC dc, int i, int* ow, int* nw)
{
	// 狭くなりすぎるのを防ぐ
	WCHAR name[MAX_PATH];
	HDITEM info = { HDI_WIDTH | HDI_TEXT };
	info.pszText = name;
	info.cchTextMax = lengthof(name);
	Header_GetItem(header, i, &info);

	*ow = info.cxy;

	// カラム名全体が表示できるだけの幅は確保しておく。
	SIZE size;
	GetTextExtentPoint32(dc, info.pszText, (int)wcslen(info.pszText), &size);
	*nw = size.cx + 16;	// マージンのぶん。適切な計算方法は不明。
}

//==========================================================================================================================
// ListView

HWND ListView_Create(HWND parent, DWORD style)
{
	return ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, null, WS_CHILD | WS_VISIBLE | style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, null, ::GetModuleHandle(null), null);
}

bool ListView_Adjust(HWND hwnd) throw()
{
	HWND header = ListView_GetHeader(hwnd);
	if (!header || !GetVisible(header))
		return true;	// hidden

	const int columns = Header_GetItemCount(header);
	if (columns <= 0)
		return true;	// no columns

	const int rows = ListView_GetItemCount(hwnd);
	if (rows <= 0)
		return false;	// no rows ... might need retry.
	HDC dc = ::GetDC(header);
	NMHEADER notify = { header, ::GetDlgCtrlID(header), HDN_DIVIDERDBLCLICK, 0, 0 };
	for (int i = 0; i < columns; ++i)
	{
		notify.iItem = i;
		::SendMessage(hwnd, WM_NOTIFY, notify.hdr.idFrom, (LPARAM)&notify);

		// 狭くなりすぎるのを防ぐ
		int ow, nw;
		Header_GetColumnWidth(header, dc, i, &ow, &nw);
		if (ow < nw)
		{
			HDITEM info = { HDI_WIDTH };
			info.cxy = nw;
			Header_SetItem(header, i, &info);
		}
	}
	::ReleaseDC(header, dc);

	return true;
}

void ListView_SelectItem_SVSI(HWND hwnd, int index, UINT svsi) throw()
{
	if (svsi & SVSI_DESELECTOTHERS)
		ListView_SetItemState(hwnd, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
	UINT lvState = LVIS_SELECTED;
	if (svsi & SVSI_FOCUSED)
		lvState |= LVIS_FOCUSED;
	ListView_SetItemState(hwnd, index, lvState, lvState);
	if (svsi & SVSI_ENSUREVISIBLE)
		ListView_EnsureVisible(hwnd, index, FALSE);
}

int ListView_GetItemText(HWND hwnd, int row, int column, WCHAR buf[], size_t bufsize)
{
	LVITEM item = { LVIF_TEXT };
	item.iItem = row;
	item.iSubItem = column;
	item.pszText = buf;
	item.cchTextMax = (int)bufsize;
	return (int)::SendMessage(hwnd, LVM_GETITEMTEXT, (WPARAM)(row), (LPARAM)&item);
}

int ListView_GetColumnCount(HWND hwnd) throw()
{
	return Header_GetItemCount(ListView_GetHeader(hwnd));
}

HRESULT ListView_GetSortedColumn(HWND hwnd, int* column, bool* ascending)
{
	ASSERT(column);
	ASSERT(ascending);
	*column = -1;
	HWND header = ListView_GetHeader(hwnd);
	if (!::IsWindow(header))
		return E_INVALIDARG;
	const int headerCount = Header_GetItemCount(header);
	*column = ListView_GetSelectedColumn(hwnd);
	*ascending = false;
	// SelectedColumn と Header の関係がおかしくなっている場合があるので
	for (int i = 0; i < headerCount; ++i)
	{
		HDITEM info = { HDI_FORMAT };
		Header_GetItem(header, i, &info);
		if (i == *column)
		{
			if ((info.fmt & (HDF_SORTUP | HDF_SORTDOWN)) == 0)
			{	// SelectedColumn なのにソートマークが付いていない！
				*column = -1; // invalid
				return E_UNEXPECTED;
			}
			*ascending = ((info.fmt & HDF_SORTDOWN) == HDF_SORTDOWN);
		}
		else if (info.fmt & (HDF_SORTUP | HDF_SORTDOWN))
		{	// SelectedColumn 以外にソートマークが付いている！
			*column = -1; // invalid
			return E_UNEXPECTED;
		}
	}
	return S_OK;
}

HRESULT ListView_SetSortedColumn(HWND hwnd, int column, bool ascending)
{
	if (column < 0)
		return E_INVALIDARG;

	HWND hwndOwner = ::GetParent(hwnd);
	HWND header = ListView_GetHeader(hwnd);
	if (!::IsWindow(header))
		return E_INVALIDARG;
	if (column >= Header_GetItemCount(header))
		return E_INVALIDARG;
	for (int i = 0; i < 10; ++i) // 変なことが起こって無限ループになるのを防ぐため。
	{
		NMLISTVIEW notify = { hwnd, ::GetDlgCtrlID(hwnd), LVN_COLUMNCLICK, -1, column };
		::SendMessage(hwndOwner, WM_NOTIFY, notify.hdr.idFrom, (LPARAM)&notify);
		int c; bool a;
		if (SUCCEEDED(ListView_GetSortedColumn(hwnd, &c, &a)) && c == column && a == ascending)
			break;
	}
	return S_OK;
}

bool ListView_HitTestColumn(HWND hwnd, int column, int x)
{
	HWND header = ListView_GetHeader(hwnd);
	if (header && ::GetVisible(hwnd))
	{
		RECT rc;
		Header_GetItemRect(header, column, &rc);
		int xx = x + GetScrollPos(hwnd, SB_HORZ);
		if (rc.left < xx && xx < rc.right)
			return true;
	}
	return false;
}

//==========================================================================================================================
// TreeView

void* TreeView_GetItemData(HWND hwnd, HTREEITEM item) throw()
{
	if (!item)
		return null;
	TVITEM tvi = { TVIF_PARAM };
	tvi.hItem = item;
	if (::SendMessage(hwnd, TVM_GETITEM, 0, (LPARAM)&tvi))
		return (void*)tvi.lParam;
	return null;
}

HTREEITEM TreeView_Insert(HWND hwnd, HTREEITEM parent, const TVITEM& item)
{
	TVINSERTSTRUCT tvis = { parent };
	tvis.item = item;
	return (HTREEITEM)::SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&tvis);
}

HTREEITEM TreeView_Insert(HWND hwnd, HTREEITEM parent, void* data, PCWSTR name, int icon) throw()
{
	TVITEM item = { TVIF_PARAM | TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE };
	item.lParam    = (LPARAM)data;
	item.cChildren = I_CHILDRENCALLBACK;
	item.pszText   = const_cast<PWSTR>(name);
	item.iImage = item.iSelectedImage = icon;
	return TreeView_Insert(hwnd, parent, item);
}

HTREEITEM TreeView_SelectItemByPos(HWND hwnd, int x, int y)
{
	TVHITTESTINFO hit = { x, y };
	if (HTREEITEM hItem = TreeView_HitTest(hwnd, &hit))
	{
		switch (hit.flags)
		{
		case TVHT_ONITEM:
		case TVHT_ONITEMICON:
		case TVHT_ONITEMLABEL:
		case TVHT_ONITEMSTATEICON:
			TreeView_SelectItem(hwnd, hItem);
			return hItem;
		}
	}
	return null;
}

//==========================================================================================================================
// ToolBar

void ToolBar_InsertItem(HWND hwnd, int image, BYTE style, BYTE state, INT_PTR text)
{
	TBBUTTON btn = { 0 };
	btn.iBitmap = image;
	btn.fsStyle = style;
	btn.fsState = state;
	btn.iString = text;
	::SendMessage(hwnd, TB_INSERTBUTTON, 0, (LPARAM)&btn);
}

int ToolBar_GetItemCount(HWND hwnd)
{
	return (int)::SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0);
}

UINT ToolBar_GetItemState(HWND hwnd, UINT nID) throw()
{
	return (UINT)::SendMessage(hwnd, TB_GETSTATE, nID, 0);
}

bool ToolBar_SetItemState(HWND hwnd, UINT nID, UINT state) throw()
{
	return 0 != ::SendMessage(hwnd, TB_SETSTATE, nID, state);
}

int ToolBar_GetHotItem(HWND hwnd) throw()
{
	return (int)::SendMessage(hwnd, TB_GETHOTITEM, 0, 0);
}

int ToolBar_SetHotItem(HWND hwnd, int index) throw()
{
	return (int)::SendMessage(hwnd, TB_SETHOTITEM, index, 0);
}

bool ToolBar_GetItemRect(HWND hwnd, int index, RECT* rc) throw()
{
	return ::SendMessage(hwnd, TB_GETITEMRECT, index, (LPARAM)rc) != 0;
}

SIZE ToolBar_GetItemSize(HWND hwnd) throw()
{
	LRESULT lResult = ::SendMessage(hwnd, TB_GETBUTTONSIZE, 0, 0);
	SIZE sz = { GET_XY_LPARAM(lResult) };
	return sz;
}

SIZE ToolBar_GetDefaultSize(HWND hwnd)
{
	RECT rc = { 0 };
	int count = ToolBar_GetItemCount(hwnd);
	::SendMessage(hwnd, TB_GETITEMRECT, count-1, (LPARAM)&rc);
	SIZE sz = { rc.right, rc.bottom };
	return sz;
}

void ToolBar_AutoSize(HWND hwnd)
{
	DWORD style = (DWORD)::GetWindowLong(hwnd, GWL_STYLE);
	if (style & CCS_NORESIZE)
	{
		SIZE szBtn = ToolBar_GetItemSize(hwnd);
		szBtn.cy += TOOLBAR_MARGIN;
		SIZE szCur = GetClientSize(hwnd);
		if (style & CCS_VERT)
			szCur.cx = math::max(szCur.cx, szBtn.cx);
		else
			szCur.cy = math::max(szCur.cy, szBtn.cy);
		SetClientSize(hwnd, szCur.cx, szCur.cy);
	}
	else
	{
		::SendMessage(hwnd, TB_AUTOSIZE, 0, 0);
	}
}

int ToolBar_HitTest(HWND hwnd, POINT pt) throw()
{
	return (int)::SendMessage(hwnd, TB_HITTEST, 0, (LPARAM)&pt);
}

int ToolBar_HitTest(HWND hwnd, int x, int y) throw()
{
	POINT pt = { x, y };
	return ToolBar_HitTest(hwnd, pt);
}

bool ToolBar_IsPressed(HWND hwnd, UINT nID) throw()
{
	return 0 != ::SendMessage(hwnd, TB_ISBUTTONPRESSED, nID, 0);
}

void ToolBar_InsertItems(HWND hwnd, size_t count, const TBBUTTON buttons[]) throw()
{
	::SendMessage(hwnd, TB_ADDBUTTONS, count, (LPARAM)buttons);
}

void ToolBar_DeleteAllItems(HWND hwnd)
{
	while (::SendMessage(hwnd, TB_DELETEBUTTON, 1, 0)) { }
}

int ToolBar_GetRows(HWND hwnd) throw()
{
	return (int) ::SendMessage(hwnd, TB_GETROWS, 0, 0);
}

void ToolBar_SetRows(HWND hwnd, size_t rows, RECT* rc)
{
	RECT rect;
	if (!rc)
		rc = &rect;
	::SendMessage(hwnd, TB_SETROWS, MAKEWPARAM(rows, FALSE), (LPARAM)rc);
}

void ToolBar_SetItemSize(HWND hwnd, int w, int h) throw()
{
	::SendMessage(hwnd, TB_SETBUTTONSIZE, 0, MAKELONG(w, h));
}

int ToolBar_GetItemInfo(HWND hwnd, UINT nID, TBBUTTONINFO* info) throw()
{
	return (int) ::SendMessage(hwnd, TB_GETBUTTONINFO, nID, (LPARAM)info);
}

void ToolBar_SetItemInfo(HWND hwnd, UINT nID, const TBBUTTONINFO* info) throw()
{
	VERIFY( ::SendMessage(hwnd, TB_SETBUTTONINFO, nID, (LPARAM)info));
}

//==========================================================================================================================
// ReBar

ReBar_Band::ReBar_Band(DWORD rbbim)
{
	memset(this, 0, sizeof(REBARBANDINFO));
	if (WINDOWS_VERSION < WINDOWS_VISTA)
		cbSize = offsetof(REBARBANDINFO, rcChevronLocation);
	else
		cbSize = sizeof(REBARBANDINFO);
	fMask = rbbim;
}

BOOL ReBar_GetBarInfo(HWND hwnd, REBARINFO* bar)
{
	return (BOOL)::SendMessage(hwnd, RB_GETBARINFO, 0, (LPARAM)bar);
}

void ReBar_SetBarInfo(HWND hwnd, const REBARINFO* bar)
{
	VERIFY(::SendMessage(hwnd, RB_SETBARINFO, 0, (LPARAM)bar));
}

void ReBar_ShowBand(HWND hwnd, int index, bool show)
{
	VERIFY(::SendMessage(hwnd, RB_SHOWBAND, index, show));
}

int ReBar_GetBandCount(HWND hwnd)
{
	return (int)::SendMessage(hwnd, RB_GETBANDCOUNT, 0, 0L);
}

bool ReBar_GetBandInfo(HWND hwnd, int index, REBARBANDINFO* band)
{
	return 0 != ::SendMessage(hwnd, RB_GETBANDINFO, index, (LPARAM)band);
}

void ReBar_SetBandInfo(HWND hwnd, int index, const REBARBANDINFO* band)
{
	VERIFY(::SendMessage(hwnd, RB_SETBANDINFO, index, (LPARAM)band));
}

int ReBar_FindBand(HWND hwnd, HWND target)
{
	ReBar_Band band(RBBIM_CHILD);
	int count = ReBar_GetBandCount(hwnd);
	for (int i = 0; i < count; ++i)
	{
		if (ReBar_GetBandInfo(hwnd, i, &band) && band.hwndChild == target)
			return i;
	}
	return -1;
}

void ReBar_InsertBand(HWND hwnd, const REBARBANDINFO* band, int index)
{
	VERIFY(::SendMessage(hwnd, RB_INSERTBAND, index, (LPARAM)band));
}

void ReBar_InsertBand(HWND hwnd, HWND child, SIZE size, int index)
{
	bool locked = ReBar_GetLocked(hwnd);
	ReBar_Band band(RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_CHILDSIZE | RBBIM_SIZE);
	band.fStyle = RBBS_BREAK | (locked ? RBBS_NOGRIPPER : RBBS_GRIPPERALWAYS);
	band.hwndChild  = child;
	band.cxIdeal    = size.cx;
	band.cxMinChild = 0;
	band.cyMinChild = size.cy;
	band.cx         = size.cx;
	ReBar_InsertBand(hwnd, &band, index);
}

bool ReBar_DeleteBand(HWND hwnd, int index)
{
	return 0 != ::SendMessage(hwnd, RB_DELETEBAND, index, 0L);
}

bool ReBar_DeleteBand(HWND hwnd, HWND child)
{
	int index = ReBar_FindBand(hwnd, child);
	if (index < 0)
		return false;
	return ReBar_DeleteBand(hwnd, index);
}

bool ReBar_SyncBands(HWND hwnd)
{
	bool changed = false;
	ReBar_Band band(RBBIM_CHILD | RBBIM_STYLE);
	const int count = ReBar_GetBandCount(hwnd);
	for (int i = 0; i < count; ++i)
	{
		if (!ReBar_GetBandInfo(hwnd, i, &band))
			continue;
		bool visible = GetVisible(band.hwndChild);
		if (visible != band.Visible)
		{
			changed = true;
			ReBar_ShowBand(hwnd, i, visible);
		}
	}
	return changed;
}

const bool ReBar_DefaultLocked = false;

bool ReBar_GetLocked(HWND hwnd)
{
	ReBar_Band band(RBBIM_STYLE);
	if (ReBar_GetBandInfo(hwnd, 0, &band))
		return band.Locked;
	return ReBar_DefaultLocked;
}

void ReBar_SetLocked(HWND hwnd, bool value)
{
	// スタイルの変更だけでは、左側の隙間を調整してくれない。
	// そのため、いったん削除し、その後で追加しなおす。
	int count = ReBar_GetBandCount(hwnd);
	if (count == 0)
		return;

	typedef std::vector<ReBar_Band> Bands;
	Bands bands;
	bands.reserve(count);
	LockRedraw pause(hwnd);
	for (int i = 0; i < count; i++)
	{
		ReBar_Band band(RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_CHILDSIZE | RBBIM_SIZE);
		if (ReBar_GetBandInfo(hwnd, 0, &band))
		{
			if (bands.empty() && band.Locked == value)
				return; // 変更の必要なし
			band.Locked = value;
			bands.push_back(band);
		}
		ReBar_DeleteBand(hwnd, 0);
	}
	// 追加しなおし
	ASSERT(ReBar_GetBandCount(hwnd) == 0);
	count = (int)bands.size();
	for (int i = 0; i < count; i++)
		ReBar_InsertBand(hwnd, &bands[i], i);
}

//==========================================================================================================================
// ToolTip

HWND ToolTip_Create(HWND parent)
{
	HWND hwnd = ::CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, null, ::GetModuleHandle(null), null);
//	::SendMessage(hwnd, WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), 0);
	return hwnd;
}

int ToolTip_GetCount(HWND hwnd) throw()
{
	return (int)::SendMessage(hwnd, TTM_GETTOOLCOUNT, 0, 0L);
}

void ToolTip_Add(HWND hwnd, const TOOLINFO* info) throw()
{
	TOOLINFO query = { sizeof(TOOLINFO) };
	query.uId = info->uId;
	query.hwnd = info->hwnd;
	if (::SendMessage(hwnd, TTM_GETTOOLINFO, 0, (LPARAM)&query))
		::SendMessage(hwnd, TTM_SETTOOLINFO, 0, (LPARAM)info);
	else
		VERIFY(::SendMessage(hwnd, TTM_ADDTOOL, 0, (LPARAM)info) );
}

void ToolTip_Add(HWND hwnd, HWND owner, HWND child, PCWSTR text, const RECT& rc) throw()
{
	TOOLINFO info = { sizeof(TOOLINFO) };
	info.uId = (UINT_PTR)child;
	info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	info.hwnd = owner;
	info.lpszText = const_cast<PWSTR>(text);
	info.rect = rc;
	ToolTip_Add(hwnd, &info);
}

void ToolTip_Remove(HWND hwnd, HWND owner, UINT nID) throw()
{
	TOOLINFO info = { sizeof(TOOLINFO) };
	info.uId = nID;
	info.hwnd = owner;
	::SendMessage(hwnd, TTM_DELTOOL, 0, (LPARAM)&info);
}

//==========================================================================================================================
// Header

int Header_HitTest(HWND hwnd, int x, int y)
{
	HDHITTESTINFO hit = { x, y };
	return (int)::SendMessage(hwnd, HDM_HITTEST, 0, (LPARAM)&hit);
}

HRESULT Header_GetSortedColumn(HWND hwnd, int* column, bool* ascending) throw()
{
	HDITEM item = { HDI_FORMAT };
	int n = Header_GetItemCount(hwnd);
	for (int i = 0; i < n; ++i)
	{
		if (Header_GetItem(hwnd, i, &item) && (item.fmt & (HDF_SORTUP | HDF_SORTDOWN)))
		{
			*column = i;
			*ascending = ((item.fmt & HDF_SORTDOWN) == HDF_SORTDOWN);
			return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT Header_SetSortedColumn(HWND hwnd, int column, bool ascending) throw()
{
	HRESULT hr = E_FAIL;
	HDITEM item = { HDI_FORMAT };
	int n = Header_GetItemCount(hwnd);
	for (int i = 0; i < n; ++i)
	{
		if (!Header_GetItem(hwnd, i, &item))
			continue;
		if (i == column)
		{
			item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
			item.fmt |= (ascending ? HDF_SORTDOWN : HDF_SORTUP);
			Header_SetItem(hwnd, i, &item);
			hr = S_OK;
		}
		else if (item.fmt & (HDF_SORTUP | HDF_SORTDOWN))
		{
			item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
			Header_SetItem(hwnd, i, &item);
		}
	}
	return hr;
}

//==========================================================================================================================
// StatusBar

bool StatusBar_GetItemRect(HWND hwnd, int part, RECT* rc)
{
	std::vector<int> widths(part+1);
	int n = (int)::SendMessage(hwnd, SB_GETPARTS, part, (LPARAM)&widths[0]);
	if (n < part)
		return false;
	GetClientRect(hwnd, rc);
	rc->left = (part > 0 ? widths[part-1] : 0);
	if (widths[part] != -1)
		rc->right = widths[part];
	return true;
}

int StatusBar_HitTest(HWND hwnd, int x) throw()
{
	if (int n = (int)::SendMessage(hwnd, SB_GETPARTS, 0, 0))
	{
		std::vector<int> widths(n);
		::SendMessage(hwnd, SB_GETPARTS, n, (LPARAM)&widths[0]);
		for (int i = 0; i < n; ++i)
		{
			if (widths[i] == -1 || x < widths[i])
				return i;
		}
	}
	return -1;
}
