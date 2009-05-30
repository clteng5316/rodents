// textview.cpp
#include "stdafx.h"
#include "commctrl.hpp"
#include "sq.hpp"

//=============================================================================
#ifdef ENABLE_TEXTVIEW

TextView::TextView() : m_internalText(false)
{
}

LRESULT TextView::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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

void TextView::onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs)
{
	// WNDCLASSEX
	::GetClassInfoEx(NULL, WC_EDIT, &wc);
	wc.lpszClassName = L"TextView";
	// CREATESTRUCT
	cs.style |= WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_MULTILINE;
}

bool TextView::onGesture(const MSG& msg)
{
	switch (msg.message)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		return false;
	}
	return __super::onGesture(msg);
}

string TextView::get_name() const throw()
{
	m_internalText = true;
	string r = __super::get_name();
	m_internalText = false;
	return r;
}

void TextView::set_name(PCWSTR value) throw()
{
	m_internalText = true;
	__super::set_name(value);
	m_internalText = false;
}

string TextView::get_text() const throw()
{
	return __super::get_name();
}

void TextView::set_text(PCWSTR value) throw()
{
	return __super::set_name(value);
}

bool TextView::get_readonly() const throw()
{
	return (GetStyle(m_hwnd) & ES_READONLY) != 0;
}

void TextView::set_readonly(bool value) throw()
{
	SendMessage(EM_SETREADONLY, value, 0);
}

POINT TextView::get_selection()
{
	POINT range;
	GetSelection((int*)&range.x, (int*)&range.y);
	return range;
}

void TextView::set_selection(POINT value)
{
	SetSelection(value.x, value.y);
}

void TextView::append(PCWSTR text) throw()
{
	SetSelection(INT_MAX, INT_MAX);
	SendMessage(EM_REPLACESEL, TRUE, (LPARAM)text);
}

void TextView::clear() throw()
{
	SendMessage(WM_CLEAR);
}

void TextView::undo() throw()
{
	SendMessage(EM_UNDO);
}

#endif
//=============================================================================
