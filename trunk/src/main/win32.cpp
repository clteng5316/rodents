// win32.cpp
#include "stdafx.h"
#include "win32.hpp"
#include "math.hpp"
#include "string.hpp"

//==========================================================================================================================
// GDI

BOOL __stdcall DeleteBitmap(HBITMAP handle) throw()	{ return ::DeleteObject(handle); }
BOOL __stdcall DeleteBrush(HBRUSH handle) throw()	{ return ::DeleteObject(handle); }
BOOL __stdcall DeleteFont(HFONT handle) throw()		{ return ::DeleteObject(handle); }
BOOL __stdcall DeletePen(HPEN handle) throw()		{ return ::DeleteObject(handle); }

SIZE BitmapSize(HBITMAP bitmap) throw()
{
	BITMAP info;
	GetObject(bitmap, sizeof(info), &info);
	SIZE sz = { info.bmWidth, info.bmHeight };
	return sz;
}

//==========================================================================================================================
// Key

#define GET_WHEEL(wParam, up, down)	((GET_WHEEL_DELTA_WPARAM(wParam) > 0) ? (up) : (down))
#define GET_XBUTTON(wParam, x1, x2)	((GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? (x1) : (x2))

INT MSG2VK(UINT msg, WPARAM wParam) throw()
{
	switch (msg)
	{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			return VK_BUTTON_L;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			return VK_BUTTON_R;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			return VK_BUTTON_M;
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			return GET_XBUTTON(wParam, VK_BUTTON_X, VK_BUTTON_Y);
		case WM_LBUTTONDBLCLK:
			return VK_BUTTON_LL;
		case WM_RBUTTONDBLCLK:
			return VK_BUTTON_RR;
		case WM_MBUTTONDBLCLK:
			return VK_BUTTON_MM;
		case WM_XBUTTONDBLCLK:
			return GET_XBUTTON(wParam, VK_BUTTON_XX, VK_BUTTON_YY);
		case WM_MOUSEWHEEL:
			return GET_WHEEL(wParam, VK_WHEEL_UP, VK_WHEEL_DOWN);
		case WM_MOUSEHWHEEL:
			return GET_WHEEL(wParam, VK_WHEEL_LEFT, VK_WHEEL_RIGHT);
		default:
			return 0;
	}
}

INT MSG2VK(const MSG& msg) throw()
{
	return MSG2VK(msg.message, msg.wParam);
}

INT MSG2MK(UINT msg, WPARAM wParam) throw()
{
	switch (msg)
	{
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_LBUTTONUP:
		return MK_LBUTTON;
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: case WM_RBUTTONUP:
		return MK_RBUTTON;
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: case WM_MBUTTONUP:
		return MK_MBUTTON;
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: case WM_XBUTTONUP:
		return GET_XBUTTON(wParam, MK_XBUTTON1, MK_XBUTTON2);
	default:
		return 0;
	}
}

INT MSG2MK(const MSG& msg) throw()
{
	return MSG2MK(msg.message, msg.wParam);
}

void SetKeyState(UINT vk, UINT mk)
{
	BYTE keys[256];
	GetKeyboardState(keys);
	BYTE vkeyControl = ((mk & MK_CONTROL) ? 0x80 : 0x00);
	BYTE vkeyShift   = ((mk & MK_SHIFT) ? 0x80 : 0x00);
	BYTE vkeyAlt     = ((mk & MK_ALTKEY) ? 0x80 : 0x00);
	BYTE vkeyLButton = ((mk & MK_LBUTTON) ? 0x80 : 0x00);
	BYTE vkeyRButton = ((mk & MK_RBUTTON) ? 0x80 : 0x00);
	BYTE vkeyMButton = ((mk & MK_MBUTTON) ? 0x80 : 0x00);
	BYTE vkeyXButton = ((mk & MK_XBUTTON1) ? 0x80 : 0x00);
	BYTE vkeyYButton = ((mk & MK_XBUTTON2) ? 0x80 : 0x00);
	keys[VK_CONTROL ] = vkeyControl;
	keys[VK_LCONTROL] = vkeyControl;
	keys[VK_RCONTROL] = vkeyControl;
	keys[VK_SHIFT ] = vkeyShift;
	keys[VK_LSHIFT] = vkeyShift;
	keys[VK_RSHIFT] = vkeyShift;
	keys[VK_MENU ] = vkeyAlt;
	keys[VK_LMENU] = vkeyAlt;
	keys[VK_RMENU] = vkeyAlt;
	keys[VK_LBUTTON ] = vkeyLButton;
	keys[VK_RBUTTON ] = vkeyRButton;
	keys[VK_MBUTTON ] = vkeyMButton;
	keys[VK_XBUTTON1] = vkeyXButton;
	keys[VK_XBUTTON2] = vkeyYButton;
	if (vk)
		keys[vk] = 0x80;
	SetKeyboardState(keys);
}

namespace
{
	inline void RestoreKey(BYTE* keys, UINT vk)
	{
		keys[vk] = (BYTE)(((UINT)GetAsyncKeyState(vk) & 0x8000U) >> 8);
	}
}

void ResetKeyState(UINT vk) throw()
{
	BYTE keys[256];
	GetKeyboardState(keys);
	RestoreKey(keys, VK_CONTROL);
	RestoreKey(keys, VK_LCONTROL);
	RestoreKey(keys, VK_RCONTROL);
	RestoreKey(keys, VK_SHIFT);
	RestoreKey(keys, VK_LSHIFT);
	RestoreKey(keys, VK_RSHIFT);
	RestoreKey(keys, VK_MENU);
	RestoreKey(keys, VK_LMENU);
	RestoreKey(keys, VK_RMENU);
	RestoreKey(keys, VK_LBUTTON);
	RestoreKey(keys, VK_RBUTTON);
	RestoreKey(keys, VK_MBUTTON);
	RestoreKey(keys, VK_XBUTTON1);
	RestoreKey(keys, VK_XBUTTON2);
	if (vk)
		RestoreKey(keys, vk);
	SetKeyboardState(keys);
}

UINT GetModifiers() throw()
{
	UINT	mk = 0;
	if (IsKeyPressed(VK_CONTROL))	{ mk |= MK_CONTROL; }
	if (IsKeyPressed(VK_SHIFT))		{ mk |= MK_SHIFT; }
	if (IsKeyPressed(VK_MENU))		{ mk |= MK_ALTKEY; }
	return mk;
}

UINT GetButtons() throw()
{
	UINT	mk = 0;
	if (IsKeyPressed(VK_LBUTTON))	{ mk |= MK_LBUTTON; }
	if (IsKeyPressed(VK_RBUTTON))	{ mk |= MK_RBUTTON; }
	if (IsKeyPressed(VK_MBUTTON))	{ mk |= MK_MBUTTON; }
	if (IsKeyPressed(VK_XBUTTON1))	{ mk |= MK_XBUTTON1; }
	if (IsKeyPressed(VK_XBUTTON2))	{ mk |= MK_XBUTTON2; }
	return mk;
}

bool IsMenuWindow(HWND hwnd)
{
	WCHAR clsname[7];
	return ::GetClassName(hwnd, clsname, 7) && wcscmp(L"#32768", clsname) == 0;
}

//==========================================================================================================================
// Window

void ActivateWindow(HWND hwnd) throw()
{
	if (::IsIconic(hwnd))
		::ShowWindow(hwnd, SW_RESTORE);
	if (!::IsWindowVisible(hwnd))
		::ShowWindow(hwnd, SW_SHOW);

	HWND hwndForeground = GetForegroundWindow();
	if (hwndForeground == hwnd)
		return;
	DWORD fg = GetWindowThreadProcessId(hwndForeground, null);
	DWORD me = GetWindowThreadProcessId(hwnd, null);
	AttachThreadInput(me, fg, TRUE);

	DWORD current, zero = 0;
	SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &current, 0);
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &zero, 0);
	SetForegroundWindow(hwnd);
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &current, 0);

	AttachThreadInput(me, fg, FALSE);
}

DWORD GetStyle(HWND hwnd) throw()
{
	return ::GetWindowLong(hwnd, GWL_STYLE);
}

DWORD GetStyleEx(HWND hwnd) throw()
{
	return ::GetWindowLong(hwnd, GWL_EXSTYLE);
}

bool GetVisible(HWND hwnd) throw()
{
	return 0 != (::GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE);
}

bool HasFocus(HWND hwnd) throw()
{
	if (!::IsWindow(hwnd))
		return false;
	HWND focus = ::GetFocus();
	return focus && (hwnd == focus) || ::IsChild(hwnd, focus);
}

namespace
{
	struct MESSAGE
	{
		UINT	msg;
		WPARAM	wParam;
		LPARAM	lParam;
		UINT	count;
	};

	BOOL CALLBACK SendMessageToChildrenCallback(HWND hwnd, LPARAM lParam)
	{
		MESSAGE* m = (MESSAGE*)lParam;
		::SendMessage(hwnd, m->msg, m->wParam, m->lParam);
		m->count++;
		return TRUE;
	}

	BOOL CALLBACK PostMessageToChildrenCallback(HWND hwnd, LPARAM lParam)
	{
		MESSAGE* m = (MESSAGE*)lParam;
		::PostMessage(hwnd, m->msg, m->wParam, m->lParam);
		m->count++;
		return TRUE;
	}
}

UINT SendMessageToChildren(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) throw()
{
	MESSAGE m = { msg, wParam, lParam };
	::EnumChildWindows(hwnd, SendMessageToChildrenCallback, (LPARAM)&m);
	return m.count;
}

UINT PostMessageToChildren(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) throw()
{
	MESSAGE m = { msg, wParam, lParam };
	::EnumChildWindows(hwnd, PostMessageToChildrenCallback, (LPARAM)&m);
	return m.count;
}

namespace
{
	static BOOL CALLBACK Invalidate(HWND hwnd, LPARAM lParam)
	{
		::InvalidateRect(hwnd, NULL, true);
		return TRUE;
	}
}

void SetRedraw(HWND hwnd, bool redraw) throw()
{
	SendMessage(hwnd, WM_SETREDRAW, (WPARAM)redraw, 0);
	if (redraw)
	{
		::InvalidateRect(hwnd, NULL, true);
		::EnumChildWindows(hwnd, Invalidate, 0);
	}
}

WNDPROC SetWindowProc(HWND hwnd, WNDPROC wndproc)
{
#ifdef _WIN64
	return (WNDPROC)::SetWindowLongPtr(hwnd, GWL_WNDPROC, LONG_PTR(wndproc));
#else
	return (WNDPROC)(LONG_PTR)::SetWindowLongPtr(hwnd, GWL_WNDPROC, (LONG)(LONG_PTR)wndproc);
#endif
}

LockRedraw::LockRedraw(HWND hwnd) : m_hwnd(hwnd)
{
	SetRedraw(m_hwnd, false);
}

LockRedraw::~LockRedraw()
{
	SetRedraw(m_hwnd, true);
}

void CenterWindow(HWND hwnd, HWND outer) throw()
{
	if (!outer)
		outer = ::GetParent(hwnd);
	RECT rcSelf;
	RECT rcParent;

	::GetWindowRect(hwnd, &rcSelf);

	if (!outer || !::IsWindowVisible(outer) || IsIconic(outer))
		::SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcParent, NULL);
	else
		::GetWindowRect(outer, &rcParent);

	int	x = (rcParent.left + rcParent.right) / 2 - RECT_W(rcSelf) / 2;
	int	y = (rcParent.top + rcParent.bottom) / 2 - RECT_H(rcSelf) / 2;
	if (!(GetStyle(hwnd) & WS_CHILD))
	{
		x = math::max(x, 0);
		y = math::max(y, 0);
	}
	::SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

SIZE GetClientSize(HWND hwnd) throw()
{
	RECT rc;
	::GetClientRect(hwnd, &rc);
	SIZE sz = { rc.right, rc.bottom };
	return sz;
}

HWND FindWindowByClass(HWND hwnd, PCWSTR name) throw()
{
	struct Finder
	{
		PCWSTR	name;
		HWND	hwnd;

		static BOOL CALLBACK Callback(HWND hwnd, LPARAM me)
		{
			Finder* i = (Finder*)me;
			WCHAR name[MAX_PATH];
			GetClassName(hwnd, name, lengthof(name));
			if (wcscmp(name, i->name))
				return true;
			i->hwnd = hwnd;
			return false;
		}
	};

	Finder i = { name };
	::EnumChildWindows(hwnd, &Finder::Callback, (LPARAM)&i);
	return i.hwnd;
}

void ScreenToClient(HWND hwnd, RECT* rc) throw()
{
	::ScreenToClient(hwnd, 0 + (POINT*)rc);
	::ScreenToClient(hwnd, 1 + (POINT*)rc);
}

void SetClientSize(HWND hwnd, int w, int h) throw()
{
	// HACK: 1回だけでは適切なサイズにならない場合があるため、2回行う。
	for (int i = 0; i < 2; ++i)
	{
		RECT rcWindow, rcClient;
		::GetWindowRect(hwnd, &rcWindow);
		::GetClientRect(hwnd, &rcClient);
		int dx = w - RECT_W(rcClient);
		int dy = h - RECT_H(rcClient);
		if (dx != 0 || dy != 0)
			::SetWindowPos(hwnd, null, 0, 0,
				RECT_W(rcWindow) + dx, RECT_H(rcWindow) + dy,
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void BoardcastMessage(UINT msg, WPARAM wParam, LPARAM lParam) throw()
{
	struct Param
	{
		UINT	msg;
		WPARAM	wParam;
		LPARAM	lParam;

		static BOOL CALLBACK Do(HWND hwnd, LPARAM lParam)
		{
			Param* m = (Param*)lParam;
			if (m->msg != WM_DESTROY)
			{
				::PostMessage(hwnd, m->msg, m->wParam, m->lParam);
				::EnumChildWindows(hwnd, &Param::Do, lParam);
			}
			else
				::DestroyWindow(hwnd);
			return TRUE;
		}
	};

	Param m = { msg, wParam, lParam };
	::EnumThreadWindows(::GetCurrentThreadId(), &Param::Do, (LPARAM)&m);
}

void PumpMessage() throw()
{
	MSG msg;
	while (::PeekMessage(&msg, null, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

void InitCommonControls(UINT flags) throw()
{
	INITCOMMONCONTROLSEX cc = { sizeof(INITCOMMONCONTROLSEX), flags };
	::InitCommonControlsEx(&cc);
}

//==========================================================================================================================
// File

int FileOperate(UINT what, FILEOP_FLAGS flags, const WCHAR src[], const WCHAR dst[])
{
	SHFILEOPSTRUCT	op = { 0 };
	op.hwnd = GetWindow();
	op.wFunc = what;
    op.fFlags = flags;
	op.pFrom = src;
	op.pTo = dst;
	return SHFileOperation(&op);
}

namespace
{
	typedef std::map<PCWSTR, HINSTANCE, Less>	DllCache;

	DllCache		theDlls;

	template < typename Map, typename Fn >
	inline void for_each_2nd(Map& m, Fn fn)
	{
		for (Map::iterator i = m.begin(); i != m.end(); ++i)
			fn(i->second);
	}
}

HINSTANCE DllLoad(PCWSTR name) throw()
{
	HINSTANCE& value = theDlls[name];
	if (!value)
		value = ::LoadLibrary(name);
	return value;
}

void DllUnload() throw()
{
	for_each_2nd(theDlls, ::FreeLibrary);
	theDlls.clear();
}
