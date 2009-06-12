// win32_commctrl.hpp
#pragma once

#include "hwnd.hpp"

#define LVM_LAST	(TV_FIRST - 1)
#define TV_LAST		(HDM_FIRST - 1)

//==========================================================================================================================
// Button

HWND	Button_Create(HWND parent, UINT nID, PCWSTR title);

template < typename T >
class ButtonT : public T
{
public:
	ButtonT(HWND hwnd = null)			{ m_hwnd = hwnd; }
	ButtonT& operator = (HWND hwnd)	{ m_hwnd = hwnd; return *this; }

	HWND	Create(HWND parent, UINT nID, PCWSTR title)
	{
		ASSERT(!m_hwnd);
		return m_hwnd = Button_Create(parent, nID, title);
	}
};

typedef ButtonT<Hwnd>	ButtonH;

//==========================================================================================================================
// Edit

HWND	Edit_Create(HWND parent, DWORD style = 0);
inline void Edit_GetSelection(HWND hwnd, int* beg, int* end) throw()	{ ::SendMessage(hwnd, EM_GETSEL, (WPARAM)beg, (LPARAM)end); }
inline void Edit_SetSelection(HWND hwnd, int beg, int end) throw()		{ ::SendMessage(hwnd, EM_SETSEL, beg, end); }

template < typename T >
class EditT : public T
{
public:
	EditT(HWND hwnd = null)			{ m_hwnd = hwnd; }
	EditT& operator = (HWND hwnd)	{ m_hwnd = hwnd; return *this; }

	HWND	Create(HWND parent, DWORD style = 0)
	{
		ASSERT(!m_hwnd);
		return m_hwnd = Edit_Create(parent, style);
	}
	void GetSelection(int* beg, int* end) const throw()	{ Edit_GetSelection(m_hwnd, beg, end); }
	void SetSelection(int beg, int end) throw()			{ Edit_SetSelection(m_hwnd, beg, end); }
};

typedef EditT<Hwnd>	EditH;

//==========================================================================================================================
// ListView

HWND	ListView_Create(HWND parent, DWORD style);
bool	ListView_Adjust(HWND hwnd) throw();
void	ListView_SelectItem_SVSI(HWND hwnd, int i, UINT svsi) throw();
int		ListView_GetColumnCount(HWND hwnd) throw();
#undef ListView_GetItemText
int		ListView_GetItemText(HWND hwnd, int row, int col, WCHAR buf[], size_t bufsize);
HRESULT	ListView_GetSortedColumn(HWND hwnd, int* column, bool* ascending);
HRESULT	ListView_SetSortedColumn(HWND hwnd, int column, bool ascending);
bool	ListView_HitTestColumn(HWND hwnd, int column, int x);

template < typename T >
class ListViewT : public T
{
public:
	ListViewT(HWND hwnd = null)			{ m_hwnd = hwnd; }
	ListViewT& operator = (HWND hwnd)	{ m_hwnd = hwnd; return *this; }

	HWND	Create(HWND parent, DWORD style)
	{
		ASSERT(!m_hwnd);
		return m_hwnd = ListView_Create(parent, style);
	}

	void	InsertColumn(const LVCOLUMN& col) throw()							{ ListView_InsertColumn(m_hwnd, col.iSubItem, &col); }

	int		GetItemCount() const throw()										{ return ListView_GetItemCount(m_hwnd); }
	int		GetSelectedCount() const throw()									{ return ListView_GetSelectedCount(m_hwnd); }
	int		GetNextItem(int start, UINT lvni) const throw()						{ return ListView_GetNextItem(m_hwnd, start, lvni); }
	BOOL	GetItemRect(int i, RECT* rc, int lvir) const throw()				{ return ListView_GetItemRect(m_hwnd, i, rc, lvir); }
	UINT	GetItemState(int i, UINT mask) throw()								{ return ListView_GetItemState(m_hwnd, i, mask); }
	void	SetItemState(int i, UINT lvis, UINT mask) throw()					{ ListView_SetItemState(m_hwnd, i, lvis, mask); }
	int		GetItemText(int i, int col, WCHAR buf[], size_t cch) const throw()	{ return ListView_GetItemText(m_hwnd, i, col, buf, cch); }
	int		GetHotItem() const throw()											{ return ListView_GetHotItem(m_hwnd); }
	void	DeleteItem(int i) throw()											{ ListView_DeleteItem(m_hwnd, i); }
	void	DeleteAllItems() throw()											{ ListView_DeleteAllItems(m_hwnd); }
	int		InsertItem(const LVITEM& item) throw()								{ return ListView_InsertItem(m_hwnd, &item); }
	void	SetItem(const LVITEM& item) throw()									{ ListView_SetItem(m_hwnd, &item); }
	void	SetItemText(int row, int col, PCWSTR value) throw()					{ ListView_SetItemText(m_hwnd, row, col, (PWSTR)value); }
	int		HitTest(LVHITTESTINFO* hit) const throw()							{ return ListView_HitTest(m_hwnd, hit); }
	int		HitTest(int x, int y) const throw()									{ LVHITTESTINFO hit = { x, y }; return HitTest(&hit); }
	bool	HitTestColumn(int column, int x) const throw()						{ return ListView_HitTestColumn(m_hwnd, column, x); }
	void	EnsureVisible(int i, bool partialOk = false) throw()				{ ListView_EnsureVisible(m_hwnd, i, partialOk); }
	HWND	GetEdit() const throw()												{ return ListView_GetEditControl(m_hwnd); }
	HWND	GetHeader() const throw()
	{
		if (HWND header = ListView_GetHeader(m_hwnd))
			if (GetVisible(header))
				return header;
		return null;
	}
	void	SelectItem(int i) throw()
	{
		SetItemState(-1, 0, LVIS_SELECTED);
		if (i >= 0)
		{
			SetItemState(i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			EnsureVisible(i);
		}
	}
	void	SetExtendedStyle(DWORD mask, DWORD style) throw()					{ ListView_SetExtendedListViewStyleEx(m_hwnd, mask, style); }
	HRESULT	GetSortedColumn(int* column, bool* ascending) const throw()			{ return ListView_GetSortedColumn(m_hwnd, column, ascending); }
	HRESULT	SetSortedColumn(int column, bool ascending) throw()					{ return ListView_SetSortedColumn(m_hwnd, column, ascending); }
	int		GetColumnCount() const throw()										{ return ListView_GetColumnCount(m_hwnd); }
	void	GetColumn(int column, LVCOLUMN* col) const throw()					{ ListView_GetColumn(m_hwnd, column, &col); }
	void	SetColumn(const LVCOLUMN& col) throw()								{ ListView_SetColumn(m_hwnd, col.iSubItem, &col); }
	UINT	GetExtendedStyle() const throw()									{ return ListView_GetExtendedListViewStyle(m_hwnd); }
};

typedef ListViewT<Hwnd>	ListViewH;

//==========================================================================================================================
// TreeView

void*		TreeView_GetItemData(HWND hwnd, HTREEITEM item) throw();
HTREEITEM	TreeView_Insert(HWND hwnd, HTREEITEM parent, const TVITEM& item);
HTREEITEM	TreeView_Insert(HWND hwnd, HTREEITEM parent, void* data, PCWSTR name, int icon = I_IMAGENONE) throw();
HTREEITEM	TreeView_SelectItemByPos(HWND hwnd, int x, int y);

template < typename T >
class TreeViewT : public T
{
public:
	TreeViewT& operator = (HWND hwnd)	{ m_hwnd = hwnd; return *this; }

public:
	void*		GetItemData(HTREEITEM item) const throw()									{ return TreeView_GetItemData(m_hwnd, item); }
	BOOL		GetItemRect(HTREEITEM item, RECT* rc, BOOL textonly = TRUE) const throw()	{ return TreeView_GetItemRect(m_hwnd, item, rc, textonly); }
	HTREEITEM	GetNextItem(HTREEITEM item, UINT flags) const throw()						{ return TreeView_GetNextItem(m_hwnd, item, flags); }
	HTREEITEM	GetChildItem(HTREEITEM item) const throw()									{ return TreeView_GetChild(m_hwnd, item); }
	void		SetItem(const TVITEM& item) throw()											{ VERIFY( TreeView_SetItem(m_hwnd, &item) ); }
	HTREEITEM	GetSelection() const throw()												{ return TreeView_GetSelection(m_hwnd); }
	HTREEITEM	GetDropHilight() const throw()												{ return TreeView_GetDropHilight(m_hwnd); }
	HTREEITEM	GetChild(HTREEITEM item) const throw()										{ return TreeView_GetChild(m_hwnd, item); }
	HTREEITEM	HitTest(TVHITTESTINFO* hit) const throw()									{ return TreeView_HitTest(m_hwnd, hit); }
	HTREEITEM	InsertItem(HTREEITEM parent, const TVITEM& item) throw()					{ return TreeView_Insert(m_hwnd, parent, item); }
	HTREEITEM	InsertItem(HTREEITEM parent, void* data, PCWSTR name = LPSTR_TEXTCALLBACK,
						   int icon = I_CHILDRENCALLBACK) throw()							{ return TreeView_Insert(m_hwnd, parent, data, name, icon); }
	BOOL		DeleteItem(HTREEITEM item) throw()											{ return TreeView_DeleteItem(m_hwnd, item); }
	void		DeleteAllItems() throw()													{ TreeView_DeleteAllItems(m_hwnd); }
	void		Expand(HTREEITEM item, UINT tve) throw()									{ TreeView_Expand(m_hwnd, item, tve); }
	void		EnsureVisible(HTREEITEM item) throw()										{ TreeView_EnsureVisible(m_hwnd, item); }
	void		SelectItem(HTREEITEM item) throw()											{ VERIFY( TreeView_SelectItem(m_hwnd, item) ); }
	HTREEITEM	SelectItem(int x, int y) throw()											{ return TreeView_SelectItemByPos(m_hwnd, x, y); }
	HWND		GetEdit() const throw()														{ return TreeView_GetEditControl(m_hwnd); }
	HIMAGELIST	SetImageList(HIMAGELIST image, INT what = TVSIL_NORMAL) throw()				{ return TreeView_SetImageList(m_hwnd, image, what); }
	HWND		EditLabel(HTREEITEM item) throw()											{ return TreeView_EditLabel(m_hwnd, item); }
	void		EndEditLabelNow(bool cancel) throw()										{ TreeView_EndEditLabelNow(m_hwnd, cancel); }
	DWORD		GetItemState(HTREEITEM item, DWORD mask) const throw()						{ return TreeView_GetItemState(m_hwnd, item, mask); }
	void		SetItemState(HTREEITEM item, DWORD data, DWORD mask) throw()				{ TreeView_SetItemState(m_hwnd, item, data, mask); }
};

typedef TreeViewT<Hwnd>	TreeViewH;

//==========================================================================================================================
// ToolBar

const int TOOLBAR_INDENT = 2;	// FIXME: magic number for XP
const int TOOLBAR_MARGIN = 8;	// FIXME: magic number for XP

int		ToolBar_GetItemCount(HWND hwnd) throw();
SIZE	ToolBar_GetItemSize(HWND hwnd) throw();
void	ToolBar_SetItemSize(HWND hwnd, int w, int h) throw();
int		ToolBar_GetItemInfo(HWND hwnd, UINT nID, TBBUTTONINFO* info) throw();
void	ToolBar_SetItemInfo(HWND hwnd, UINT nID, const TBBUTTONINFO* info) throw();
UINT	ToolBar_GetItemState(HWND hwnd, UINT nID) throw();
bool	ToolBar_SetItemState(HWND hwnd, UINT nID, UINT state) throw();
int		ToolBar_GetHotItem(HWND hwnd) throw();
int		ToolBar_SetHotItem(HWND hwnd, int index) throw();
bool	ToolBar_GetItemRect(HWND hwnd, int i, RECT* rc) throw();
void	ToolBar_InsertItem(HWND hwnd, int image, BYTE style, BYTE state, INT_PTR text = -1) throw();
void	ToolBar_InsertItems(HWND hwnd, size_t count, const TBBUTTON buttons[]) throw();
void	ToolBar_DeleteAllItems(HWND hwnd);
SIZE	ToolBar_GetDefaultSize(HWND hwnd) throw();
void	ToolBar_AutoSize(HWND hwnd) throw();
int		ToolBar_GetRows(HWND hwnd) throw();
void	ToolBar_SetRows(HWND hwnd, size_t rows, RECT* rc = null);
int		ToolBar_HitTest(HWND hwnd, POINT pt) throw();
int		ToolBar_HitTest(HWND hwnd, int x, int y) throw();
bool	ToolBar_IsPressed(HWND hwnd, UINT nID) throw();

template < typename T >
class ToolBarT : public T
{
public:
	int		GetItemCount() const throw()												{ return ToolBar_GetItemCount(m_hwnd); }
	UINT	GetItemState(UINT nID) const throw()											{ return ToolBar_GetItemState(m_hwnd, nID); }
	bool	SetItemState(UINT nID, UINT state) throw()									{ return ToolBar_SetItemState(m_hwnd, nID, state); }
	int		GetItemInfo(UINT nID, TBBUTTONINFO* info) const throw()		{ return ToolBar_GetItemInfo(m_hwnd, nID, info); }
	void	SetItemInfo(UINT nID, const TBBUTTONINFO* info) throw()		{ ToolBar_SetItemInfo(m_hwnd, nID, info); }
	int		GetHotItem() const throw()													{ return ToolBar_GetHotItem(m_hwnd); }
	int		SetHotItem(int index) throw()												{ return ToolBar_SetHotItem(m_hwnd, index); }
	bool	GetItemRect(int i, RECT* rc) const throw()									{ return ToolBar_GetItemRect(m_hwnd, i, rc); }
	void	InsertItem(int image, BYTE style, BYTE state, INT_PTR text = -1) throw()	{ ToolBar_InsertItem(m_hwnd, image, style, state, text); }
	void	InsertItems(size_t count, const TBBUTTON buttons[]) throw()					{ ToolBar_InsertItems(m_hwnd, count, buttons); }
	void	DeleteAllItems() throw()													{ ToolBar_DeleteAllItems(m_hwnd); }
	SIZE	GetItemSize() const throw()													{ return ToolBar_GetItemSize(m_hwnd); }
	void	SetItemSize(int w, int h) throw()							{ ToolBar_SetItemSize(m_hwnd, w, h); }
	SIZE	GetDefaultSize() const throw()												{ return ToolBar_GetDefaultSize(m_hwnd); }
	void	AutoSize() throw()															{ ToolBar_AutoSize(m_hwnd); }
	int		HitTest(POINT pt) const throw()												{ return ToolBar_HitTest(m_hwnd, pt); }
	int		HitTest(int x, int y) const throw()											{ return ToolBar_HitTest(m_hwnd, x, y); }
	bool	IsPressed(UINT nID) const throw()											{ return ToolBar_IsPressed(m_hwnd, nID); }
	int		GetRows() const throw()										{ return ToolBar_GetRows(m_hwnd); }
	void	SetRows(size_t rows, RECT* rc = null) throw()								{ ToolBar_SetRows(m_hwnd, rows, rc); }
};

//==========================================================================================================================
// ReBar

struct ReBar_Band : REBARBANDINFO
{
	ReBar_Band(DWORD rbbim);

	__declspec(property(get=get_visible, put=set_visible)) bool visible;
	bool get_visible() const		{ return (fStyle & RBBS_HIDDEN) == 0; }
	void set_visible(bool value)
	{
		if (value)
			fStyle &= ~RBBS_HIDDEN;
		else
			fStyle |= RBBS_HIDDEN;
	}

	__declspec(property(get=get_newline, put=set_newline)) bool newline;
	bool get_newline() const		{ return (fStyle & RBBS_BREAK) != 0; }
	void set_newline(bool value)
	{
		if (value)
			fStyle |= RBBS_BREAK;
		else
			fStyle &= ~RBBS_BREAK;
	}

	__declspec(property(get=get_locked, put=set_locked)) bool locked;
	bool get_locked() const		{ return (fStyle & RBBS_NOGRIPPER) != 0; }
	void set_locked(bool value)
	{
		if (value)
		{	// locked, without grip
			fStyle &= ~RBBS_GRIPPERALWAYS;
			fStyle |= RBBS_NOGRIPPER;
		}
		else
		{	// unlocked, with grip
			fStyle |= RBBS_GRIPPERALWAYS;
			fStyle &= ~RBBS_NOGRIPPER;
		}
	}
};

BOOL	ReBar_GetBarInfo(HWND hwnd, REBARINFO* bar);
void	ReBar_SetBarInfo(HWND hwnd, const REBARINFO* bar);
void	ReBar_ShowBand(HWND hwnd, int i, bool show);
int		ReBar_GetBandCount(HWND hwnd);
bool	ReBar_GetBandInfo(HWND hwnd, int i, REBARBANDINFO* band);
void	ReBar_SetBandInfo(HWND hwnd, int i, const REBARBANDINFO* band);
int		ReBar_FindBand(HWND hwnd, HWND target);
void	ReBar_InsertBand(HWND hwnd, const REBARBANDINFO* band, int i = -1);
void	ReBar_InsertBand(HWND hwnd, HWND child, SIZE size, int i = -1);
bool	ReBar_DeleteBand(HWND hwnd, int i);
bool	ReBar_DeleteBand(HWND hwnd, HWND child);
bool	ReBar_SyncBands(HWND hwnd);
bool	ReBar_GetLocked(HWND hwnd);
void	ReBar_SetLocked(HWND hwnd, bool value);

template < typename T >
class ReBarT : public T
{
public:
	BOOL	GetBarInfo(REBARINFO* bar) const					{ return ReBar_GetBarInfo(m_hwnd, bar); }
	void	SetBarInfo(const REBARINFO* bar)					{ ReBar_SetBarInfo(m_hwnd, bar); }
	void	ShowBand(int i, bool show)							{ ReBar_ShowBand(m_hwnd, i, show); }
	int		GetBandCount() const								{ return ReBar_GetBandCount(m_hwnd); }
	bool	GetBandInfo(int i, REBARBANDINFO* band) const		{ return ReBar_GetBandInfo(m_hwnd, i, band); }
	void	SetBandInfo(int i, const REBARBANDINFO* band)		{ ReBar_SetBandInfo(m_hwnd, i, band); }
	int		FindBand(HWND target) const							{ return ReBar_FindBand(m_hwnd, target); }
	void	InsertBand(const REBARBANDINFO* band, int i = -1)	{ ReBar_InsertBand(m_hwnd, band, i); }
	void	InsertBand(HWND child, SIZE size, int i = -1)		{ ReBar_InsertBand(m_hwnd, child, size, i); }
	bool	DeleteBand(int i)									{ return ReBar_DeleteBand(m_hwnd, i); }
	bool	DeleteBand(HWND child)								{ return ReBar_DeleteBand(m_hwnd, child); }
	bool	SyncBands()											{ return ReBar_SyncBands(m_hwnd); }
	bool	GetLocked() const									{ return ReBar_GetLocked(m_hwnd); }
	void	SetLocked(bool value)								{ ReBar_SetLocked(m_hwnd, value); }
};

//==========================================================================================================================
// StatusBar

bool	StatusBar_GetItemRect(HWND hwnd, int part, RECT* rc);
int		StatusBar_HitTest(HWND hwnd, int x) throw();

template < typename T >
class StatusBarT : public T
{
public:
	int		GetParts() const throw()							{ return (int)::SendMessage(m_hwnd, SB_GETPARTS, 0, 0); }
	int		GetParts(size_t ln, int parts[]) const throw()		{ return (int)::SendMessage(m_hwnd, SB_GETPARTS, ln, (LPARAM)parts); }
	void	SetParts(size_t ln, const int parts[]) throw()		{ VERIFY( ::SendMessage(m_hwnd, SB_SETPARTS, ln, (LPARAM)parts) ); }
	size_t	GetItemTextLength(int part) const throw()			{ return (ssize_t)::SendMessage(m_hwnd, SB_GETTEXTLENGTH, part, 0); }
	DWORD	GetItemText(int part, WCHAR text[]) const throw()	{ return (DWORD)::SendMessage(m_hwnd, SB_GETTEXT, part, (LPARAM)text); }
	HICON	GetItemIcon(int part) const throw()					{ return (HICON)::SendMessage(m_hwnd, SB_GETICON, part, 0); }
	bool	GetItemRect(int part, RECT* rc) const throw()		{ return StatusBar_GetItemRect(m_hwnd, part, rc); }
	void	GetItemTip(int part, WCHAR text[]) const throw()	{ ::SendMessage(m_hwnd, SB_GETTIPTEXT, part, (LPARAM)text); }
	void	SetItemText(int part, PCWSTR text) throw()			{ VERIFY( ::SendMessage(m_hwnd, SB_SETTEXT, part, (LPARAM)text) ); }
	void	SetItemIcon(int part, HICON icon) throw()			{ VERIFY( ::SendMessage(m_hwnd, SB_SETICON, part, (LPARAM)icon) ); }
	void	SetItemTip(int part, PCWSTR text) throw()			{ ::SendMessage(m_hwnd, SB_SETTIPTEXT, part, (LPARAM)text); }
	int		HitTest(int x) const throw()						{ return StatusBar_HitTest(m_hwnd, x); }
};

//==========================================================================================================================
// ToolTip

HWND	ToolTip_Create(HWND parent);
int		ToolTip_GetCount(HWND hwnd) throw();
void	ToolTip_Add(HWND hwnd, const TOOLINFO* info) throw();
void	ToolTip_Add(HWND hwnd, HWND owner, HWND child, PCWSTR text, const RECT& rc) throw();
void	ToolTip_Remove(HWND hwnd, HWND owner, UINT nID) throw();

template < typename T >
class ToolTipT : public T
{
public:
	HWND	Create(HWND parent)
	{
		ASSERT(!m_hwnd);
		return m_hwnd = ToolTip_Create(parent);
	}
	int		GetCount() const throw()											{ return ToolTip_GetCount(m_hwnd); }
	void	Add(const TOOLINFO* info) throw()									{ ToolTip_Add(m_hwnd, info); }
	void	Add(HWND owner, HWND child, PCWSTR text, const RECT& rc) throw()	{ ToolTip_Add(m_hwnd, owner, child, text, rc); }
	void	Remove(HWND owner, UINT nID) throw()								{ ToolTip_Remove(m_hwnd, owner, nID); }
};

typedef ToolTipT<Hwnd>	ToolTipH;

//==========================================================================================================================
// Header

int		Header_HitTest(HWND hwnd, int x, int y) throw();
HRESULT	Header_GetSortedColumn(HWND hwnd, int* column, bool* ascending) throw();
HRESULT	Header_SetSortedColumn(HWND hwnd, int column, bool ascending) throw();

template < typename T >
class HeaderT : public T
{
public:
	HeaderT(HWND hwnd = null) : T(hwnd)		{}

	int		GetItemCount() const throw()										{ return Header_GetItemCount(m_hwnd); }
	BOOL	GetItemRect(int index, RECT* rc) const throw()						{ return Header_GetItemRect(m_hwnd, index, rc); }
	BOOL	GetItem(int index, HDITEM* item) const throw()						{ return Header_GetItem(m_hwnd, index, item); }
	int		InsertItem(const HDITEM& item, int index = INT_MAX) const throw()	{ return Header_InsertItem(m_hwnd, index, &item); }
	BOOL	DeleteItem(int index) throw()										{ return Header_DeleteItem(m_hwnd, index); }
	void	DeleteAllItems() throw()											{ while (DeleteItem(0)) {} }
	int		HitTest(int x, int y) const throw()									{ return Header_HitTest(m_hwnd, x, y); }
	HRESULT	GetSortedColumn(int* column, bool* ascending) const throw()			{ return Header_GetSortedColumn(m_hwnd, column, ascending); }
	HRESULT	SetSortedColumn(int column, bool ascending) throw()					{ return Header_SetSortedColumn(m_hwnd, column, ascending); }
	HIMAGELIST	GetImageList() const throw()									{ return Header_GetImageList(m_hwnd); }
	HIMAGELIST	SetImageList(HIMAGELIST image) throw()							{ return Header_SetImageList(m_hwnd, image); }
};

typedef HeaderT<Hwnd>	HeaderH;
