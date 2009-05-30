// hwnd.hpp
#pragma once

#include "win32.hpp"

class Hwnd
{
protected:
	HWND	m_hwnd;

public:
	Hwnd(HWND hwnd = NULL) : m_hwnd(hwnd)	{}

	operator HWND () const throw()		{ return m_hwnd; }
	HWND	get_Handle() const throw()	{ return m_hwnd; }
	__declspec(property(get=get_Handle)) HWND Handle;

	Hwnd& operator = (HWND hwnd)		{ m_hwnd = hwnd; return *this; }

	static HWND from(IUnknown* obj) throw();
	static HWND from(IOleWindow* obj) throw();

	HWND	get_parent() const throw();

	bool	get_visible() const throw();
	void	set_visible(bool value) throw();

	int		get_width() const throw()			{ return get_size().cx; }
	void	set_width(int value) throw();

	int		get_height() const throw()			{ return get_size().cy; }
	void	set_height(int value) throw();

	POINT	get_location() const throw();
	void	set_location(POINT value) throw()	{ move(value.x, value.y); }

	SIZE	get_size() const throw();
	void	set_size(SIZE value) throw()		{ resize(value.cx, value.cy); }

	RECT	get_bounds() const;
	void	set_bounds(const RECT& value) throw();

	BOOL	IsWindow() const throw()			{ return ::IsWindow(m_hwnd); }

	void	close() throw()						{ PostMessage(WM_CLOSE); }
	void	cut() throw()						{ SendMessage(WM_CUT); }
	void	copy() throw()						{ SendMessage(WM_COPY); }
	void	paste() throw()						{ SendMessage(WM_PASTE); }
	void	move(int x, int y) throw();
	void	resize(int w, int h) throw();
	void	maximize() throw();
	void	minimize() throw();
	void	restore() throw();

	LRESULT	SendMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const throw()
	{
		if (!m_hwnd)
			return 0;
		return ::SendMessage(m_hwnd, msg, wParam, lParam);
	}

	BOOL PostMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const throw()
	{
		if (!m_hwnd)
			return false;
		return ::PostMessage(m_hwnd, msg, wParam, lParam);
	}

	void	SendCommand(UINT wID) throw()					{ SendMessage(WM_COMMAND, MAKEWPARAM(wID, 0), 0); }
	void	PostCommand(UINT wID) throw()					{ PostMessage(WM_COMMAND, MAKEWPARAM(wID, 0), 0); }
	void	SetTimer(UINT wID, UINT delay) throw()			{ ::SetTimer(m_hwnd, wID, delay, NULL); }
	void	KillTimer(UINT wID) throw()						{ ::KillTimer(m_hwnd, wID); }
	int		GetTextLength() const throw()					{ return ::GetWindowTextLength(m_hwnd); }
	int		GetText(WCHAR value[], int ln) const throw()	{ return ::GetWindowText(m_hwnd, value, ln); }
	void	SetText(PCWSTR value) throw()					{ ::SetWindowText(m_hwnd, value); }
	HFONT	GetFont() const throw()					{ return (HFONT)SendMessage(WM_GETFONT, 0, 0); }
	void	SetFont(HFONT value) throw()			{ SendMessage(WM_SETFONT, (WPARAM)value, TRUE); }

	void	ModifyStyle(DWORD dwRemove, DWORD dwAdd, DWORD dwRemoveEx = 0, DWORD dwAddEx = 0) throw();
	void	DisableIME() throw()			{ ::ImmAssociateContext(m_hwnd, NULL); }

	BOOL	IsParentOf(HWND hwnd) const		{ return ::IsChild(m_hwnd, hwnd); }

	POINT	ScreenToClient(POINT pt) const	{ ::ScreenToClient(m_hwnd, &pt); return pt; }
	POINT	ClientToScreen(POINT pt) const	{ ::ClientToScreen(m_hwnd, &pt); return pt; }
	void	ScreenToClient(POINT* pt) const	{ VERIFY( ::ScreenToClient(m_hwnd, pt) ); }
	void	ClientToScreen(POINT* pt) const	{ VERIFY( ::ClientToScreen(m_hwnd, pt) ); }
	//void	ScreenToClient(RECT* rc) const	{ ::ScreenToClient(m_hwnd, rc); }
	//void	ClientToScreen(RECT* rc) const	{ ::ClientToScreen(m_hwnd, rc); }

	void	Close() throw();
	void	Dispose() throw()				{ ::DestroyWindow(m_hwnd); }

	void	Activate() throw();
	HWND	SetFocus() throw()					{ return ::SetFocus(m_hwnd); }
	HWND	SetCapture()						{ return ::SetCapture(m_hwnd); }
	void	ReleaseCapture()					{ if (::GetCapture() == m_hwnd) { ::ReleaseCapture(); } }

	HDC		GetDC() throw()						{ return ::GetDC(m_hwnd); }
	void	ReleaseDC(HDC dc) throw()			{ ::ReleaseDC(m_hwnd, dc); }
	void	Invalidate() throw()				{ ::InvalidateRect(m_hwnd, NULL, TRUE); }
	void	Invalidate(const RECT& rc) throw()	{ ::InvalidateRect(m_hwnd, &rc, TRUE); }
	void	TrackMouseEvent(DWORD tme, DWORD time) throw();
	void	SetLayerBitmap(HBITMAP bitmap, BYTE alpha = 255) throw();
};
