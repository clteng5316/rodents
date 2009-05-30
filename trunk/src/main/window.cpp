// window.cpp
#include "stdafx.h"
#include "sq.hpp"
#include "string.hpp"
#include "ref.hpp"
#include "window.hpp"

extern void HidePreview(HWND hwnd);

SQInteger get_hotkeys(sq::VM v)
{
	sq_pushregistrytable(v);
	v.push(L"hotkeys");
	if SQ_FAILED(sq_rawget(v, -2))
		return SQ_ERROR;
	return 1;
}

namespace
{
	static std::vector<ATOM>	m_hotkeys;

	static UINT MK2MOD(UINT mods)
	{
		UINT ret = 0;
		if (mods & MK_CONTROL)	ret |= MOD_CONTROL;
		if (mods & MK_SHIFT)	ret |= MOD_SHIFT;
		if (mods & MK_ALTKEY)	ret |= MOD_ALT;
		if (mods & MK_WINKEY)	ret |= MOD_WIN;
		return ret;
	}

	static UINT16 MOD2MK(UINT mods)
	{
		UINT16 ret = 0;
		if (mods & MOD_CONTROL)	ret |= MK_CONTROL;
		if (mods & MOD_SHIFT)	ret |= MK_SHIFT;
		if (mods & MOD_ALT)		ret |= MK_ALTKEY;
		if (mods & MOD_WIN)		ret |= MK_WINKEY;
		return ret;
	}

	static void HandleHotKeys(int mods) throw()
	{
		if (sq::VM v = sq::vm)
		{
			SQInteger top = v.gettop();
			if (get_hotkeys(v) > 0)
			{
				v.push(mods);
				if SQ_SUCCEEDED(sq_get(v, -2))
				{
					sq_pushroottable(v);
					sq::call(v, 1, SQFalse);
				}
			}
			v.settop(top);
		}
	}
}

void UnregisterHotKeys()
{
	size_t count = m_hotkeys.size();
	for (size_t i = 0; i < count; ++i)
	{
		ATOM atom = m_hotkeys[i];
		::UnregisterHotKey(NULL, (int)atom);
		::GlobalDeleteAtom(atom);
	}
	m_hotkeys.clear();
}

void RegisterHotKeys()
{
	if (sq::VM v = sq::vm)
	{
		SQInteger top = v.gettop();
		HWND hwnd = GetWindow();
		if (::IsWindow(hwnd) && get_hotkeys(v) > 0)
		{
			try
			{
				ssize_t ln = wcslen(PATH_ROOT);
				if (ln > 230)
					ln -= 230; // ATOM には255文字制限があるため、長すぎる場合は削る。
				// register hotkeys
				foreach (v, -1)
				{
					if (sq_gettype(v, -2) == OT_INTEGER)
					{
						int mods = v.get<int>(-2);
						WCHAR	name[256];
						swprintf_s(name, L"%s:%04X", PATH_ROOT + ln, mods);
						if (ATOM atom = ::GlobalAddAtom(name))
							if (::RegisterHotKey(hwnd, (int)atom, MK2MOD(mods >> 8), mods & 0xFF))
								m_hotkeys.push_back(atom);
					}
				}
			}
			catch (sq::Error&)
			{
				ASSERT(!"Error in RegisterHotKeys");
				UnregisterHotKeys();
			}
		}
		v.settop(top);
		sq_reseterror(v);
	}
}

SQInteger set_hotkeys(sq::VM v)
{
	UnregisterHotKeys();
	sq_pushregistrytable(v);
	v.push(L"hotkeys");
	sq_push(v, 2);
	sq_rawset(v, -3);
	RegisterHotKeys();
	return 0;
}

void sq::push(HSQUIRRELVM v, HWND value) throw()
{
	if (Window* w = Window::from(value))
		sq::push(v, w->get_self());
	else
		sq_pushnull(v);
}

static Window* m_headobj;

Window* Window::headobj()		{ return m_headobj; }
Window* Window::nextobj() const	{ return m_nextobj; }

Window::Window()
{
	m_pressed = false;
	m_updated = true;
	m_dock = CENTER;
}

Window::~Window()
{
	if (IsWindow())
	{
		ASSERT(!"ウィンドウが破棄されていない");
		::DestroyWindow(m_hwnd);
	}
}

void Window::constructor(sq::VM v)
{
	// attach to existing window
	sq_getstackobj(v, 1, &m_self);
	if (sq_gettype(v, 2) == OT_USERPOINTER)
	{
		HWND hwnd;
		if (SQ_SUCCEEDED(sq_getuserpointer(v, 2, (void**)&hwnd)) &&
			::IsWindow(hwnd) && !Window::from(hwnd) &&
			SetWindowSubclass(hwnd, &Window::onMessage, 0, (DWORD_PTR)this))
		{
			sq_addref(sq::vm, &m_self);
			m_hwnd = hwnd;
			// slist
			m_nextobj = m_headobj;
			m_headobj = this;
			return;
		}
		else
			throw sq::Error(L"Window.constructor");
	}

	Window* parent	= v.get_if_exists<Window*>(2);

	// WNDCLASSEX
	WNDCLASSEX	wc = { sizeof(WNDCLASSEX) };

	// CREATESTRUCT
	CREATESTRUCT cs = { 0 };
	cs.hwndParent = (parent ? parent->m_hwnd : null);
	cs.x = cs.y = cs.cx = cs.cy = CW_USEDEFAULT;
	if (cs.hwndParent)
	{
		cs.style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		cs.dwExStyle = WS_EX_CLIENTEDGE;
	}
	else
	{
		cs.style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		cs.dwExStyle = WS_EX_WINDOWEDGE;
	}
	cs.lpCreateParams = this;

	onCreate(wc, cs);

	// Add prefix "APPNAME."
	WCHAR	clsname[MAX_PATH];
	swprintf_s(clsname, L"%s.%s", APPNAME, wc.lpszClassName);
	wc.lpszClassName = clsname;

	CreateWindow(wc, cs, &Window::onMessage);
	ASSERT(this == Window::from(m_hwnd));
}

void Window::onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs)
{
	wc.hCursor = ::LoadCursor(null, IDC_ARROW);
	wc.lpszClassName = L"Window";
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = ::GetModuleHandle(null);
}

void Window::finalize()
{
	if (IsWindow())
		::DestroyWindow(m_hwnd);
	ASSERT(!m_hwnd);
	m_hwnd = null;
	__super::finalize();
}

BYTE Window::get_dock() const
{
	return m_dock;
}

static inline DWORD DockToCCS(BYTE dock) throw()
{
	switch (dock)
	{
	case NORTH: return CCS_TOP;
	case SOUTH: return CCS_BOTTOM;
	case WEST:  return CCS_LEFT;
	case EAST:  return CCS_RIGHT;
	default:    return 0;
	}
}

#define CCS_MASK	(CCS_TOP | CCS_BOTTOM | CCS_LEFT | CCS_RIGHT)

void Window::set_dock(BYTE value) throw()
{
	m_dock = value;
	if (IsWindow())
	{
		ModifyStyle(CCS_MASK, DockToCCS(value));
		if (Window* parent = from(get_parent()))
			parent->update();
		update();
	}
}

string Window::get_font() throw()
{
	return null;
}

void Window::set_font(PCWSTR value) throw()
{
	HFONT font = GetStockFont(DEFAULT_GUI_FONT);
	LOGFONT info;
	GetObject(font, sizeof(info), &info);
//	info.lfHeight = -size;
	wcscpy_s(info.lfFaceName, value);
	HFONT fontNew = CreateFontIndirect(&info);
	SendMessage(WM_SETFONT, (WPARAM)fontNew, 0);
}

SIZE Window::get_DefaultSize() const throw()
{
	SIZE s = { 0, 0 };
	switch (m_dock)
	{
	case NORTH:
	case SOUTH:
		s.cy = get_height();
		break;
	case EAST:
	case WEST:
		s.cx = get_width();
		break;
	}
	return s;
}

string Window::get_name() const throw()
{
	if (!IsWindow())
		return null;
	int len = ::GetWindowTextLength(m_hwnd) + 1;
	string buffer(sizeof(WCHAR) * len);
	::GetWindowText(m_hwnd, buffer, len);
	return buffer;
}

void Window::set_name(PCWSTR value) throw()
{
	if (IsWindow())
		::SetWindowText(m_hwnd, value);
}

SQInteger Window::get_placement(sq::VM v)
{
	try
	{
		WINDOWPLACEMENT place = { sizeof(WINDOWPLACEMENT) };
		GetWindowPlacement(m_hwnd, &place);
		bool	zoomed = (place.showCmd == SW_MAXIMIZE);
		bool	vertical = ((GetStyle(m_hwnd) & CCS_VERT) != 0);
		v.newtable();
		v.push(L"bounds");
		v.newarray((LONG*)&place.rcNormalPosition, 4);
		v.newslot(-3);
		v.push(L"zoomed");
		v.push(zoomed);
		v.newslot(-3);
		v.push(L"vertical");
		v.push(vertical);
		v.newslot(-3);
		return 1;
	}
	catch (sq::Error&)
	{
		return SQ_ERROR;
	}
}

SQInteger Window::set_placement(sq::VM v)
{
	try
	{
		RECT	bounds = { 0, 0, 0, 0 };
		bool	zoomed = false;
		bool	vertical = false;

		v.push(L"bounds");
		if SQ_SUCCEEDED(sq_get(v, 2))
		{
			v.getarray(-1, (LONG*)&bounds, 4);
			sq_pop(v, 1);
		}
		v.push(L"zoomed");
		if SQ_SUCCEEDED(sq_get(v, 2))
		{
			sq::get(v, -1, &zoomed);
			sq_pop(v, 1);
		}
		v.push(L"vertical");
		if SQ_SUCCEEDED(sq_get(v, 2))
		{
			sq::get(v, -1, &vertical);
			sq_pop(v, 1);
		}

		WINDOWPLACEMENT place = { sizeof(WINDOWPLACEMENT) };
		GetWindowPlacement(m_hwnd, &place);
		if (!IsRectEmpty(&bounds))
			place.rcNormalPosition = bounds;
		place.showCmd = (zoomed ? SW_SHOWMAXIMIZED : SW_RESTORE);
		SetWindowPlacement(m_hwnd, &place);
		if (vertical)
			ModifyStyle(0, CCS_VERT);
		else
			ModifyStyle(CCS_VERT, 0);
		return 0;
	}
	catch (sq::Error&)
	{
		return SQ_ERROR;
	}
}

Window* Window::from(HWND hwnd)
{
	DWORD_PTR data;
	if (hwnd && ::GetWindowSubclass(hwnd, &Window::onMessage, 0, &data) && ((Window*)data)->m_hwnd == hwnd)
		return (Window*)data;
	return null;
}

bool Window::onEvent(PCWSTR name, Handler handler, LPARAM arg)
{
	SQBool ret = false;
	if (sq::VM v = sq::vm)
	{
		SQInteger	top = v.gettop();
		v.push(m_self);
		v.push(name);
		if (SQ_SUCCEEDED(sq_get(v, -2)) && sq_gettype(v, -1) == OT_CLOSURE)
		{
			SQInteger	n = v.gettop();
			sq_push(v, -2);
			if (handler)
				handler(v, arg);
			sq::call(v, v.gettop() - n, SQTrue);
			sq_tobool(v, -1, &ret);
		}
		sq_reseterror(v);
		v.settop(top);
	}
	return ret == SQTrue;
}

LRESULT Window::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		if (NMHDR* n = (NMHDR*)lParam)
		{
			ASSERT(wParam == n->idFrom);
			if (Window* w = Window::from(n->hwndFrom))
				return w->onNotify(this, *n);
		}
		break;
	case WM_SETFOCUS:
		if (!m_focus.IsWindow())
			m_focus = onFocus();
		if (m_focus)
			m_focus.SetFocus();
		break;
	case WM_NOTIFYFOCUS:
		set_focus((HWND)lParam);
		return 0;
	case WM_SHNOTIFY:
		SHNotifyMessage(wParam, lParam);
		return 0;
	case WM_UPDATEUISTATE:
		if (m_updated)
		{
			if (IsWindowVisible(m_hwnd))
				doUpdate();
			m_updated = false;
		}
		return 0;
	case WM_COMMAND:
		if (Window* from = Window::from((HWND)lParam))
		{
			NMHDR nm = { from->Handle, LOWORD(wParam), WM_COMMAND };
			from->onNotify(this, nm);
			return 0;
		}
		break;
	case WM_SIZE:
		if ((wParam == 0 || wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) &&
			get_visible())
		{
			if (Window* parent = from(get_parent()))
				parent->update();
			doUpdate();
		}
		break;
	case WM_SHOWWINDOW:
		if (lParam == 0) // by ShowWindow
		{
			BOOL shown = (wParam != 0);
			if (Window* parent = from(get_parent()))
				parent->update();
			if (shown)
				doUpdate();
		}
		break;
	case WM_PARENTNOTIFY:
		switch (LOWORD(wParam))
		{
		case WM_CREATE:
			update();
			break;
		case WM_DESTROY:
			if (HWND hwnd = (HWND)lParam)
			{
				if (!m_focus.IsWindow())
					m_focus = null;
				else if (m_focus == hwnd)
				{
					m_focus = null;
					// 次のウィンドウをフォーカスする
					if (Window* w = from(::GetWindow(hwnd, GW_HWNDNEXT)))
						if (w->get_dock() == CENTER)
							m_focus = w->Handle;
				}
				if (HasFocus(hwnd))
					PostMessage(WM_NOTIFYFOCUS, 0, 0);
			}
			update();
			break;
		}
		break;
	case WM_CONTEXTMENU:
		if (m_hwnd == GetWindow() && lParam != 0xFFFFFFFF)
		{
			POINT	ptScreen = { GET_XY_LPARAM(lParam) };
			POINT	ptClient = ScreenToClient(ptScreen);
			// タイトルバー上の右クリックを乗っ取らないように、クライアント領域上の場合のみ。
			if (ptClient.x >= 0 && ptClient.y >= 0)
			{
				onContextMenu(ptScreen.x, ptScreen.y);
				return 0;
			}
		}
		break;
	case WM_SETTINGCHANGE:
		PostMessageToChildren(m_hwnd, msg, wParam, lParam);
		break;
	case WM_HOTKEY:
	{
		int vkey = (UINT8)HIWORD(lParam);
		int mods = MOD2MK(LOWORD(lParam));
		HandleHotKeys(vkey | mods << 8);
		break;
	}
	case WM_ACTIVATEAPP:
	case WM_ENTERSIZEMOVE:
		HidePreview(null);
		break;
	case WM_ENDSESSION:
		if (!wParam)
			break;
		// fall through
	case WM_CLOSE:
		if (onEvent(L"onClose"))
			return 0;
		break;
	case WM_NCCREATE:
		return TRUE;	// 直接呼び出されているため。
	}
	return ::DefSubclassProc(m_hwnd, msg, wParam, lParam);
}

LRESULT Window::onNotify(Window* from, NMHDR& nm)
{
	if (nm.code != WM_COMMAND)
		return ::DefSubclassProc(from->m_hwnd, WM_NOTIFY, nm.idFrom, (LPARAM)&nm);
	else // WM_COMMANDは直接呼ばれる。
		return 0;
}

void Window::set_focus(HWND hwnd)
{
	if (IsParentOf(hwnd))
	{
		m_focus = hwnd;
		if (m_dock == CENTER)
			if (Window* parent = from(get_parent()))
				parent->set_focus(m_hwnd);
	}
	else
		SetFocus();
}

LRESULT CALLBACK Window::onMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	Window* self = (Window*)data;

	switch (msg)
	{
	case WM_NCCREATE:
		sq_addref(sq::vm, &self->m_self);
		self->m_hwnd = hwnd;
		// slist
		self->m_nextobj = m_headobj;
		m_headobj = self;
		break;
	case WM_NCDESTROY:
		LRESULT lResult = self->onMessage(msg, wParam, lParam);
		::RemoveWindowSubclass(hwnd, &Window::onMessage, id);
		self->m_hwnd = null;
		// slist
		if (m_headobj == self)
			m_headobj = null;
		for (Window* i = m_headobj; i; i = i->m_nextobj)
		{
			if (i->m_nextobj == self)
				i->m_nextobj = self->m_nextobj;
		}
		self->m_nextobj = null;
		sq_release(sq::vm, &self->m_self);	// 最速ならこの呼び出しで delete this される
		return lResult;
	}

	if (self->m_hwnd == hwnd)
		return self->onMessage(msg, wParam, lParam);
	else
		return ::DefWindowProc(hwnd, msg, wParam, lParam);
};

void Window::press_with_mods(UINT vk, UINT mk)
{
	if (vk >= 256)
		return;
	SetKeyState(vk, mk);
	SendMessage(WM_KEYDOWN, vk, 0);
	ResetKeyState(vk);
}

void Window::press(UINT key)
{
	press_with_mods(key & 0xFF, (key & ~0xFF) >> 8);
}

bool Window::handlePress(const MSG& msg)
{
	for (HWND hwnd = msg.hwnd; hwnd; hwnd = ::GetAncestor(hwnd, GA_PARENT))
	{
		if (Window* w = Window::from(hwnd))
			return w->onPress(msg);
	}
	return false;
}

static void onPress(sq::VM v, const MSG* msg)
{
	v.push((UINT)msg->wParam | GetModifiers() << 8);
}

bool Window::onPress(const MSG& msg)
{
	// 無限ループを防ぐため。
	if (m_pressed)
		return false;

	bool	ret;
	m_pressed = true;
	ret = onEvent(L"onPress", ::onPress, &msg);
	m_pressed = false;
	if (ret)
		return true;

	// 親へ転送。
	if (Window* parent = Window::from(get_parent()))
		return parent->onPress(msg);
	return false;
}

bool Window::onGesture(const MSG& msg)
{
	// 親へ転送。
	if (Window* parent = Window::from(get_parent()))
		return parent->onGesture(msg);

	// 左クリックは禁止
	switch (msg.message)
	{
	case WM_LBUTTONDOWN:
		return false;
	}

	// 非クライアント領域なら禁止
	RECT rc;
	::GetClientRect(m_hwnd, &rc);
	POINT pt = POINT_FROM_LPARAM(msg.lParam);
	if (m_hwnd != msg.hwnd)
		::MapWindowPoints(msg.hwnd, m_hwnd, &pt, 1);
	return PtInRect(&rc, pt) != 0;
}

//================================================================================

int	GESTURE_NAPTIME = 100; // [ms] これ以上時間が経った場合は新たなジェスチャとみなす（↑↑など）
int	GESTURE_TIMEOUT = 400; // [ms] これ以上停止していると、ジェスチャをキャンセルする。
// XXX: 短くしすぎると、たまにドラッグが効かない。Windows内部でタイムアウトパラメータを持っているのかも？

namespace
{
	class Gesture
	{
	private:
		Window*		m_window;
		HWND		m_hwnd;
		INT			m_vk; // 始動ボタンのVK表現
		INT			m_mk; // 始動ボタンのMK表現
		POINT		m_startpos;
		POINT		m_lastpos;
		DWORD		m_starttime;
		DWORD		m_lasttime;
		std::vector<WCHAR>	m_dirs;
		bool		m_done;	// 少なくとも1つのジェスチャを行っていればtrue。

	public:
		Gesture(Window* window, const MSG& msg);
		bool	next();

	private:
		bool	invoke(const MSG& msg) throw();
		void	restart(const MSG* msg = null) throw();
		void	terminate(const MSG& msg);
		bool	onMove();
		bool	GetMessageWithTimeout(MSG* msg);
	};
}

//==========================================================================================================================

namespace
{
	inline UINT MK2VK(UINT mk)
	{
		switch (mk)
		{
		case MK_LBUTTON : return VK_BUTTON_L;
		case MK_RBUTTON : return VK_BUTTON_R;
		case MK_MBUTTON : return VK_BUTTON_M;
		case MK_XBUTTON1: return VK_BUTTON_X;
		case MK_XBUTTON2: return VK_BUTTON_Y;
		default         : ASSERT(0); return 0;
		}
	}
}

Gesture::Gesture(Window* window, const MSG& msg) :
	m_window(window), m_hwnd(msg.hwnd), m_done(false)
{
	ASSERT(m_window);
	ASSERT(m_hwnd);
	m_vk = MSG2VK(msg.message, msg.wParam);
	m_mk = MSG2MK(msg);
	restart(&msg);
}

void Gesture::restart(const MSG* msg) throw()
{
	if (msg)
	{
		m_startpos.x = GET_X_LPARAM(msg->lParam);
		m_startpos.y = GET_Y_LPARAM(msg->lParam);
		::ClientToScreen(msg->hwnd, &m_startpos);
	}
	else
	{
		m_done = true;
		::GetCursorPos(&m_startpos);
		if (!::IsWindow(m_hwnd))
			m_hwnd = ::WindowFromPoint(m_startpos);
		if (!::IsWindow(m_hwnd))
			m_hwnd = m_window->Handle;
	}
	m_lastpos = m_startpos;
	m_starttime = m_lasttime = ::GetTickCount();
	m_dirs.clear();	// すべてのケースで空のハズだが、念のため。
	m_window->SetCapture();
}

bool Gesture::next()
{
	if (m_vk == 0)
	{
		m_window->ReleaseCapture();
		::GetCursorPos(&m_lastpos);
		MouseDrag(m_mk, m_startpos, m_lastpos);
		return false;
	}

	MSG msg = { 0 }; // 初回呼び出し時にはゼロ初期化すること！
	while (GetMessageWithTimeout(&msg))
	{
		if (::GetCapture() != m_window->Handle)
			return false;

		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if (onMove())
				return true;	// continue
			break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
			if (MSG2MK(msg) == m_mk)
			{	// 始動ボタンが離された。
				terminate(msg);
				return false;	// finish
			}
			else
			{	// 始動ボタン以外が離された。
				// fall down
			}
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
			m_window->ReleaseCapture();
			invoke(msg);
			if (!GetButtons())
				return false;
			restart();
			return true;
		default:
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			break;
		}
	}

	// タイムアウトした

	// ボタンが押されていなければ、そのまま終わる
	if (!IsKeyPressed(MK2VK(m_mk)))
		return false;

	// ジェスチャ中の操作を巻き戻し、ドラッグしていたことにする。
	m_vk = 0;
	return true;
}

namespace
{
	struct GestureArgs
	{
		UINT	vk;
		PCWSTR	dirs;
		size_t	len;
	};

	static void onGesture(sq::VM v, LPARAM param)
	{
		const GestureArgs* args = (const GestureArgs*)param;
		v.push(args->vk);
		sq::push(v, args->dirs, args->len);
	}
}

void Gesture::terminate(const MSG& msg)
{
	m_window->ReleaseCapture();

	GestureArgs args = { m_vk, (m_dirs.empty() ? L"" : &m_dirs[0]), m_dirs.size() };
	if (!m_window->onEvent(L"onGesture", onGesture, (LPARAM)&args) && !m_done) 
	{
		// ジェスチャコマンドは無かったので、通常のクリックをエミュレートする。
		MouseUp(MSG2MK(msg), GetCursorPos());
	}
}

bool Gesture::invoke(const MSG& msg) throw()
{
	UINT vkey = MSG2VK(msg);
	if (vkey == 0)
		return false;
	TRACE(L"Gesture.invoke (0x%x)", vkey);
	GestureArgs args = { vkey, L"", 0 };
	return m_window->onEvent(L"onGesture", onGesture, (LPARAM)&args);
}

bool Gesture::onMove()
{
	POINT pt = GetCursorPos();

	int dx = pt.x-m_lastpos.x, dy = pt.y-m_lastpos.y;
	if (abs(dx) < ::GetSystemMetrics(SM_CXDRAG) && abs(dy) < ::GetSystemMetrics(SM_CYDRAG))
		return false; // 不感距離
	m_lastpos = pt;

	static const WCHAR DIR[2][2] = { { 'U', 'L' }, { 'R', 'D' } };
	WCHAR dir = DIR[dy+dx>0 ? 1 : 0][dy-dx>0 ? 1 : 0];

	DWORD now = ::GetTickCount();
	if (m_dirs.empty() || m_dirs.back() != dir || (now - m_lasttime) > (DWORD)GESTURE_NAPTIME)
		m_dirs.push_back(dir);	// 新しいジェスチャ
	m_lasttime = now;
	return true;
}

bool Gesture::GetMessageWithTimeout(MSG* msg)
{
	size_t ln = m_dirs.size();

	if (ln == 0)
	{
		::GetMessage(msg, null, 0, 0);
		m_lasttime = ::GetTickCount();
		return true;
	}

	// 静止してから GESTURE_TIMEOUT の時間が経過すると、ジェスチャではないと判定される。
	for (;;)
	{
		DWORD timeout = GESTURE_TIMEOUT - (::GetTickCount() - m_lasttime);
		if ((INT32)timeout <= 0)
			return false;
		if (::PeekMessage(msg, null, 0, 0, PM_REMOVE))
			break;
		if (WAIT_TIMEOUT == MsgWaitForMultipleObjects(0, null, false, timeout, QS_INPUT))
			return false;
	}

	switch (msg->message)
	{
	case WM_MOUSEMOVE:
	case WM_TIMER:
	case WM_PAINT:
	case WM_UPDATEUISTATE:
		break;	// タイマをリセットしない。
	default:
		m_lasttime = ::GetTickCount();
		break;
	}
	return true;
}

bool Window::handleGesture(const MSG& msg)
{
	Window* w = null;
	for (HWND hwnd = msg.hwnd; hwnd; hwnd = ::GetAncestor(hwnd, GA_PARENT))
		if ((w = Window::from(hwnd)) != null)
			break;
	if (!w || !w->onGesture(msg))
		return false;	// ジェスチャは受け入れられなかった。

	// ジェスチャループ
	TRACE(L"Gesture.Start");
	Gesture gesture(w, msg);
	while (gesture.next())
	{
		// ジェスチャ中のフィードバック
//		notify(on_notify, gesture.status());
	}
	// ジェスチャ終了。メッセージを消去する。
//	notify(on_notify, L"");
	TRACE(L"Gesture.Stop");
	return true;
}

Window* Window::onDragOver(DWORD keys, POINT ptScreen, DWORD* effect)
{
	if (Window* parent = from(get_parent()))
		return parent->onDragOver(keys, ptScreen, effect);
	if (effect)
		*effect = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;
	return this;
}

void Window::onDragDrop(IDataObject* data, DWORD keys, POINT ptScreen, DWORD* effect)
{
	ref<IShellItemArray> items;
	if SUCCEEDED(PathsCreate(&items, data))
	{
		onEvent(L"onDrop", items.ptr);
		*effect = DROPEFFECT_COPY;
	}
	else
		*effect = 0;
}

void Window::onDragLeave()
{
}
