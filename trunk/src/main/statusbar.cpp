// statusbar.cpp
#include "stdafx.h"
#include "commctrl.hpp"
#include "sq.hpp"

//=============================================================================
#ifdef ENABLE_STATUSBAR

StatusBar::StatusBar() : m_internalText(false)
{
	set_dock(SOUTH);
}

void StatusBar::onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs)
{
	BYTE dock = get_dock();
	InitCommonControls(ICC_BAR_CLASSES);
	// WNDCLASSEX
	::GetClassInfoEx(NULL, STATUSCLASSNAME, &wc);
	wc.lpszClassName = L"StatusBar";
	// CREATESTRUCT
	cs.style |= CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CSS_BOTTOM;
	if (dock == SOUTH)
		cs.style |= SBARS_SIZEGRIP;
	cs.dwExStyle = 0;
}

LRESULT StatusBar::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_GETTEXT:
	case WM_GETTEXTLENGTH:
	case WM_SETTEXT:
		if (m_internalText)
			return ::CallWindowProc(DefWindowProc, m_hwnd, msg, wParam, lParam);
		break;
	}
	return __super::onMessage(msg, wParam, lParam);
}

string StatusBar::get_name() const throw()
{
	m_internalText = true;
	string r = __super::get_name();
	m_internalText = false;
	return r;
}

void StatusBar::set_name(PCWSTR value) throw()
{
	m_internalText = true;
	__super::set_name(value);
	m_internalText = false;
}

string StatusBar::get_text() const throw()
{
	return __super::get_name();
}

void StatusBar::set_text(PCWSTR value) throw()
{
	return __super::set_name(value);
}

#endif
//=============================================================================
