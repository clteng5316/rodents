// rebar.cpp
#include "stdafx.h"
#include "commctrl.hpp"
#include "math.hpp"
#include "sq.hpp"
#include "string.hpp"
#include "resource.h"

//=============================================================================
#ifdef ENABLE_REBAR

void ReBar::onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs)
{
	InitCommonControls(ICC_COOL_CLASSES);
	// WNDCLASSEX
	::GetClassInfoEx(NULL, REBARCLASSNAME, &wc);
	wc.lpszClassName = L"ReBar";
	// CREATESTRUCT
	cs.style |= RBS_DBLCLKTOGGLE | RBS_VARHEIGHT | RBS_BANDBORDERS | CCS_NODIVIDER | CCS_NOPARENTALIGN;
	cs.dwExStyle = WS_EX_COMPOSITED;
	// dock
	set_dock(NORTH);
}

void ReBar::onUpdate(RECT& bounds) throw()
{
	SyncBands();
}

LRESULT ReBar::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PARENTNOTIFY:
		switch (LOWORD(wParam))
		{
		case WM_CREATE:
		case WM_DESTROY:
			onChildChange(LOWORD(wParam) == WM_CREATE, (HWND)lParam);
			break;
		}
		break;
//	case WM_SETTINGCHANGE:
//		OnReset();
//		break;
	case WM_CREATE:
		if (LRESULT lResult = __super::onMessage(msg, wParam, lParam))
			return lResult;
		DisableIME();
		REBARINFO info = { sizeof(REBARINFO) };
		SetBarInfo(&info);
//		OnReset();
		return 0;
	}
	return __super::onMessage(msg, wParam, lParam);
}

LRESULT ReBar::onNotify(Window* from, NMHDR& nm)
{
	switch (nm.code)
	{
	case RBN_HEIGHTCHANGE:
		from->update();
		break;
	}
	return __super::onNotify(from, nm);
}

bool ReBar::get_locked() const		{ return GetLocked(); }
void ReBar::set_locked(bool value)	{ SetLocked(value); }

SQInteger ReBar::get_placement(sq::VM v)
{
	v.newtable();

	v.newslot(-1, L"locked", get_locked());

	v.push(L"children");
	v.newarray();
	ReBar_Band band(RBBIM_SIZE | RBBIM_STYLE | RBBIM_CHILD);
	for (int i = 0; GetBandInfo(i, &band); i++)
	{
		if (Window* child = Window::from(band.hwndChild))
		{
			if (string name = child->get_name())
			{
				v.newtable();
				v.newslot(-1, L"name", name);
				v.newslot(-1, L"width", band.cx);
				v.newslot(-1, L"visible", band.visible);
				v.newslot(-1, L"newline", band.newline);
				v.append(-2);
			}
		}
	}
	v.newslot(-3);

	return 1;
}

SQInteger ReBar::set_placement(sq::VM v)
{
	bool	locked = v.getslot(2, L"locked");

	v.getslot(2, L"children");

	// すでに追加されたバンドを並び替えるよりも、新しく追加したほうが楽なので。
	typedef std::vector<ReBar_Band> Bands;
	ReBar_Band	band(RBBIM_SIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE);
	Bands		bands;

	try
	{
		while (GetBandInfo(0, &band))
		{
			bands.push_back(band);
			DeleteBand(0);
		}

		foreach (v, -1)
		{
			SQInteger	idx = v.abs(-1);
			PCWSTR		name = v.getslot(idx, L"name");
			int			width = v.getslot(idx, L"width");
			bool		visible = v.getslot(idx, L"visible");
			bool		newline = v.getslot(idx, L"newline");

			for (Bands::iterator i = bands.begin(); i != bands.end(); ++i)
			{
				Window* child = Window::from(i->hwndChild);
				if (child && wcscmp(name, child->get_name()) == 0)
				{
					i->cx = width;
					i->visible = visible;
					i->newline = newline;
					break;
				}
			}
		}
	}
	catch (sq::Error&)
	{
		ASSERT(0);
		sq_reseterror(v);
	}

	// バントは取得順に再追加する。
	for (Bands::iterator i = bands.begin(); i != bands.end(); ++i)
		InsertBand(&*i);

	set_locked(locked);
	return 0;
}

void ReBar::onChildChange(bool create, Hwnd child) throw()
{
	if (child.get_parent() == m_hwnd)
	{
		if (create)
		{
			SIZE size;
			if (Window* w = from(child))
				size = w->get_DefaultSize();
			else
				size.cx = size.cy = 32; // とりあえず
			child.ModifyStyle(0, 0, WS_EX_COMPOSITED, WS_EX_TRANSPARENT);
			InsertBand(child, size);
		}
		else
		{
			DeleteBand(child);
		}
	}
}

#endif
//=============================================================================

namespace
{
	class DWP
	{
	private:
		HDWP	m_dwp;
	public:
		DWP(int n)		{ m_dwp = ::BeginDeferWindowPos(n); }
		~DWP()			{ ::EndDeferWindowPos(m_dwp); }
		void operator () (HWND hwnd, const RECT& r)
		{
			m_dwp = ::DeferWindowPos(m_dwp, hwnd, null, RECT_XYWH(r), SWP_NOACTIVATE | SWP_NOZORDER);
		}
	};

	const DWORD		INCLUDE_HIDDEN	= 1;
	const DWORD		INCLUDE_CENTER	= 2;
	const DWORD		FIRST_CENTER	= 4;

	class Layout
	{
	public:
		struct Child
		{
			HWND	hwnd;
			BYTE	dock;

			struct Sort
			{
				bool operator () (const Child& lhs, const Child& rhs) const throw()
				{
					return lhs.dock < rhs.dock;
				}
			};
		};
		typedef std::vector<Child> Children;

	private:
		HWND		m_hwnd;
		Children	m_children;
		DWORD		m_flags;

	public:
		const Children& Enum(HWND hwnd, DWORD flags);
		void Do(HWND hwnd, RECT bounds);

	private:
		static BOOL CALLBACK OnEnum(HWND hwnd, LPARAM lParam);
	};
}

BOOL CALLBACK Layout::OnEnum(HWND hwnd, LPARAM lParam)
{
	Layout* self = (Layout*)lParam;
	if (self->m_hwnd == ::GetParent(hwnd))
	{
		Window* w = Window::from(hwnd);
		if (w && ((self->m_flags & INCLUDE_HIDDEN) || w->get_visible()))
		{
			Child c = { hwnd, w->get_dock() };
			switch (c.dock)
			{
			case NONE:
				break;
			case CENTER:
				if (self->m_flags & (FIRST_CENTER | INCLUDE_CENTER))
					self->m_children.push_back(c);
				if (self->m_flags & FIRST_CENTER)
					return FALSE;
				break;
			default:
				if ((self->m_flags & FIRST_CENTER) == 0)
					self->m_children.push_back(c);
				break;
			}
		}
	}
	return TRUE;
}

const Layout::Children& Layout::Enum(HWND hwnd, DWORD flags)
{
	m_hwnd = hwnd;
	m_children.clear();
	m_flags = flags;
	::EnumChildWindows(m_hwnd, &Layout::OnEnum, (LPARAM)this);
	return m_children;
}

void Layout::Do(HWND hwnd, RECT bounds)
{
	Enum(hwnd, INCLUDE_CENTER);

	std::stable_sort(m_children.begin(), m_children.end(), Child::Sort());

	DWP dwp((int)m_children.size());
	Children::const_iterator i;

	// dock = NSWE
	for (i = m_children.begin(); i != m_children.end(); ++i)
	{
		if (i->dock == CENTER)
			break;

		RECT r1, r2;
		::GetWindowRect(i->hwnd, &r1);
		SIZE sz = Window::from(i->hwnd)->get_DefaultSize();
		int w = math::max(RECT_W(r1), sz.cx);
		int h = math::max(RECT_H(r1), sz.cy);

		switch (i->dock)
		{
		case NORTH:
			SetRect(&r2, bounds.left, bounds.top, bounds.right, bounds.top + h);
			bounds.top += h;
			break;
		case SOUTH:
			SetRect(&r2, bounds.left, bounds.bottom - h, bounds.right, bounds.bottom);
			bounds.bottom -= h;
			break;
		case WEST:
			SetRect(&r2, bounds.left, bounds.top, bounds.left + w, bounds.bottom);
			bounds.left += w;
			break;
		case EAST:
			SetRect(&r2, bounds.right - w, bounds.top, bounds.right, bounds.bottom);
			bounds.right -= w;
			break;
		default:
			ASSERT(0);
			continue;
		}

		if (r1 != r2)
			dwp(i->hwnd, r2);
	}

	// dock = CENTER
	int numCenters = (int)(m_children.end() - i);
	if (numCenters == 0)
		return;

	// 縦横のレイアウトを切り替えたい場合はここで
	int horz, vert;
	if (GetStyle(hwnd) & CCS_VERT)
	{
		horz = 1;
		vert = numCenters;
	}
	else
	{
		horz = numCenters;
		vert = 1;
	}

	const int BORDER_W = ::GetSystemMetrics(SM_CXFIXEDFRAME);
	const int BORDER_H = ::GetSystemMetrics(SM_CYFIXEDFRAME);
	const int PADDING_W = 0;
	const int PADDING_H = 0;

	int width  = (RECT_W(bounds) - PADDING_W*2) - BORDER_W * (horz-1);
	int height = (RECT_H(bounds) - PADDING_H*2) - BORDER_H * (vert-1);
	int h = 0, v = 0;

	for (size_t n = 0; i != m_children.end(); ++i, ++n)
	{
		ASSERT(i->dock == CENTER);

		const int x = bounds.left + h * width / horz + h * BORDER_W + PADDING_W;
		const int y = bounds.top + v * height / vert + v * BORDER_H + PADDING_H;

		RECT r = { x, y, x+width/horz, y+height/vert };
		if (h == horz-1)
		{	// 一番右端は、枠ぴったりに
			r.right = bounds.right - PADDING_W;
		}
		if (v == vert-1)
		{	// 一番下端は、枠ぴったりに
			r.bottom = bounds.bottom - PADDING_H;
		}
		dwp(i->hwnd, r);
		if (++h == horz)
		{
			h = 0;
			++v;
		}
	}
}

void Window::doUpdate() throw()
{
	RECT bounds;
	GetClientRect(m_hwnd, &bounds);
	onUpdate(bounds);
}

void Window::onUpdate(RECT& bounds)
{
	if (!::IsRectEmpty(&bounds))
	{
		Layout	layout;
		layout.Do(m_hwnd, bounds);
	}
	m_updated = false;
}

HWND Window::onFocus() throw()
{
	Layout layout;
	const Layout::Children& children = layout.Enum(m_hwnd, FIRST_CENTER);
	return children.empty() ? null : children.front().hwnd;
}

namespace
{
	void ContextMenu_Append(HMENU menu, HWND hwnd, int id)
	{
		WCHAR name[MAX_PATH+4] = L"&N: ";
		PWSTR s = name + 4;
		if (::InternalGetWindowText(hwnd, s, MAX_PATH) <= 0)
			::GetClassName(hwnd, s, MAX_PATH);
		int n = GetMenuItemCount(menu);
		if (n < 9)
		{
			name[1] = (WCHAR)(L'1' + n);
			s = name;
		}
		AppendMenu(menu, MF_STRING | (GetVisible(hwnd) ? MF_CHECKED : 0), id, s);
	}

	int ContextMenu_AppendReBar(HMENU menu, HWND hwnd, std::vector<HWND>& children)
	{
		int count = ReBar_GetBandCount(hwnd);
		for (int i = 0; i < count; ++i)
		{
			ReBar_Band band(RBBIM_CHILD);
			ReBar_GetBandInfo(hwnd, i, &band);
			children.push_back(band.hwndChild);
			ContextMenu_Append(menu, band.hwndChild, (int)children.size());
		}
		return count;
	}
}

void Window::onContextMenu(int x, int y)
{
	POINT ptScreen = { x, y };

	MenuH popup;
	std::vector<HWND> items;
	std::vector<HWND> rebar;

	Layout layout;
	const Layout::Children& children = layout.Enum(m_hwnd, INCLUDE_HIDDEN);

	size_t count = children.size();
	if (count == 0)
		return;

	popup = CreatePopupMenu();
	for (size_t index = 0; index < count; ++index)
	{
		HWND h = children[index].hwnd;
		Window* w = Window::from(h);
		if (!w || w->get_dock() == CENTER)
			continue;
		items.push_back(h);
		if (dynamic_cast<ReBar*>(w))
		{
			ContextMenu_AppendReBar(popup, h, items);
			rebar.push_back(h);
		}
		else
			ContextMenu_Append(popup, h, (int)items.size());
	}

	enum
	{
		ID_LOCK	= 1000,
		ID_HORZ,
		ID_VERT
	};

	// ロック
	AppendMenu(popup, MF_SEPARATOR, 0, 0);
	bool locked = false;
	if (!rebar.empty())
	{
		locked = ReBar_GetLocked(rebar.front());
		AppendMenu(popup, MF_STRING | (locked ? MF_CHECKED : 0), ID_LOCK, _S(STR_LOCKBARS));
		AppendMenu(popup, MF_SEPARATOR, 0, 0);
	}

	// レイアウト
	if (GetStyle(m_hwnd) & CCS_VERT)
	{
		AppendMenu(popup, MF_STRING, ID_HORZ, _S(STR_TILEHORZ));
		AppendMenu(popup, MF_STRING | MF_CHECKED, ID_VERT, _S(STR_TILEVERT));
	}
	else
	{
		AppendMenu(popup, MF_STRING | MF_CHECKED, ID_HORZ, _S(STR_TILEHORZ));
		AppendMenu(popup, MF_STRING, ID_VERT, _S(STR_TILEVERT));
	}

	// メニュー
	if (popup)
	{
		UINT cmd = ::TrackPopupMenu(popup, TPM_RIGHTBUTTON | TPM_RETURNCMD, ptScreen.x, ptScreen.y, 0, m_hwnd, 0);
		if (0 < cmd && cmd <= items.size())
		{
			HWND h = items[cmd - 1];
			::ShowWindow(h, GetVisible(h) ? SW_HIDE : SW_SHOW);
		}
		else switch (cmd)
		{
		case ID_LOCK:
			for (size_t i = 0; i < rebar.size(); ++i)
				ReBar_SetLocked(rebar[i], !locked);
			break;
		case ID_HORZ:
			ModifyStyle(CCS_VERT, 0);
			update();
			break;
		case ID_VERT:
			ModifyStyle(0, CCS_VERT);
			update();
			break;
		}
	}
}

SQInteger Window::get_children(sq::VM v)
{
	Layout layout;
	const Layout::Children& children = layout.Enum(m_hwnd, INCLUDE_HIDDEN | INCLUDE_CENTER);

	v.newarray();
	for (Layout::Children::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		if (Window* w = Window::from(i->hwnd))
		{
			v.push(w->get_self());
			v.append(-2);
		}
	}

	return 1;
}

//================================================================================

RECT Window::get_CenterArea() const throw()
{
	RECT	bounds;
	GetClientRect(m_hwnd, &bounds);

	Layout layout;
	const Layout::Children& children = layout.Enum(m_hwnd, 0);

	size_t count = children.size();
	for (size_t index = 0; index < count; ++index)
	{
		HWND h = children[index].hwnd;
		Window* w = Window::from(h);
		SIZE sz = w->get_DefaultSize();
		switch (w->get_dock())
		{
		case NORTH:
			bounds.top += sz.cy;
			break;
		case SOUTH:
			bounds.bottom -= sz.cy;
			break;
		case WEST:
			bounds.left += sz.cx;
			break;
		case EAST:
			bounds.right -= sz.cx;
			break;
		}
	}

	return bounds;
}
