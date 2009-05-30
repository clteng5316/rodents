// commctrl.cpp
#include "stdafx.h"
#include "commctrl.hpp"
#include "sq.hpp"

void sq::push(HSQUIRRELVM v, IImageList* value)
{
	sq::push(v, value, null, null);
}

//=============================================================================

void SubclassSingleLineEdit(HSQUIRRELVM v_, HWND hwnd)
{
	if (sq::VM v = v_)
	{
		SQInteger top = v.gettop();
		sq_pushroottable(v);
		v.push(L"editor");
		if SQ_SUCCEEDED(sq_rawget(v, -2))
		{
			sq_pushroottable(v);
			sq_pushuserpointer(v, hwnd);
			sq::call(v, 2, false);
			if (Window *w = Window::from(hwnd))
				w->set_dock(NONE);
		}
		sq_reseterror(v);
		v.settop(top);
	}
}

//=============================================================================

static PCWSTR item_string(sq::VM v, SQInteger idx, PCWSTR attr)
{
	idx = v.abs(idx);
	if (sq_gettype(v, idx) & SQOBJECT_DELEGABLE)
	{
		v.push(attr);
		if SQ_SUCCEEDED(sq_get(v, idx))
		{
			switch (sq_gettype(v, -1))
			{
			case OT_NULL:
				return null;
			case OT_STRING:
				return v.get<PCWSTR>(-1);
			default:
				return v.tostring(-1);
			}
		}
		sq_reseterror(v);
	}
	return null;
}

static int item_integer(sq::VM v, SQInteger idx, PCWSTR name, int defval)
{
	idx = v.abs(idx);
	if (sq_gettype(v, idx) & SQOBJECT_DELEGABLE)
	{
		v.push(name);
		if SQ_SUCCEEDED(sq_get(v, idx))
		{
			switch (sq_gettype(v, -1))
			{
			case OT_BOOL:
				return (int) v.get<bool>(-1);
			case OT_INTEGER:
				return v.get<int>(-1);
			}
			sq_pop(v, 1);
		}
		sq_reseterror(v);
	}
	return defval;
}

PCWSTR item_name(HSQUIRRELVM v_, SQInteger idx)
{
	if (sq::VM v = v_)
	{
		ASSERT(idx > 0);
		SQObjectType type = sq_gettype(v, idx);
		if (type == OT_NULL)
			return NULL;
		else if (type == OT_STRING)
			return v.get<PCWSTR>(idx);
		else if (PCWSTR name = item_string(v, idx, L"name"))
			return name;
		else
			return v.tostring(idx);
	}
	return null;
}

int item_icon(HSQUIRRELVM v, SQInteger idx)
{
	return item_integer(v, idx, L"icon", I_IMAGENONE);
}

int item_check(HSQUIRRELVM v, SQInteger idx)
{
	return item_integer(v, idx, L"check", -1);
}

//=============================================================================
