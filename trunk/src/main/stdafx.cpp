// stdafx.cpp

#include "stdafx.h"

#ifdef _DEBUG

void TRACE(PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR buffer[1024];
	vswprintf_s(buffer, format, args);
	OutputDebugString(buffer);
	OutputDebugString(L"\n");
	va_end(args);
}

void TRACE_V(PCWSTR format, va_list args) throw()
{
	WCHAR buffer[1024];
	vswprintf_s(buffer, format, args);
	OutputDebugString(buffer);
	OutputDebugString(L"\n");
	va_end(args);
}

static void TRACE_HWND(HWND hwnd, int level, int x, int y) throw()
{
	for (int i = 0; i < level; ++i)
		OutputDebugString(L"  ");

	WCHAR cls[MAX_PATH];
	::GetClassName(hwnd, cls, MAX_PATH);
	RECT rc;
	::GetWindowRect(hwnd, &rc);
	TRACE(L"[%0x08p] %s (%d,%d) (%d,%d)", hwnd, cls, rc.left - x, rc.top - y, RECT_WH(rc));
	for (HWND c = ::GetWindow(hwnd, GW_CHILD); c; c = ::GetWindow(c, GW_HWNDNEXT))
		TRACE_HWND(c, level + 1, x, y);
}

void TRACE_HWND(HWND hwnd) throw()
{
	RECT rc;
	::GetWindowRect(hwnd, &rc);
	TRACE_HWND(hwnd, 0, rc.left, rc.top);
}

void TRACE_STACK(HSQUIRRELVM v) throw()
{
	SQInteger	top = sq_gettop(v);
	for (SQInteger idx = 1; idx <= top; idx++)
	{
		HSQOBJECT	obj;
		sq_getstackobj(v, idx, &obj);

		switch (sq_gettype(v, idx))
		{
		case OT_NULL:
			TRACE(L"stack[%d] : null", idx);
			break;
		case OT_INTEGER:
			TRACE(L"stack[%d] : integer", idx);
			break;
		case OT_FLOAT:
			TRACE(L"stack[%d] : float", idx);
			break;
		case OT_BOOL:
			TRACE(L"stack[%d] : bool", idx);
			break;
		case OT_STRING:
			TRACE(L"stack[%d] : string", idx);
			break;
		case OT_TABLE:
			TRACE(L"stack[%d] : table(0x%p)", idx, obj._unVal.pTable);
			break;
		case OT_ARRAY:
			TRACE(L"stack[%d] : array", idx);
			break;
		case OT_USERDATA:
			TRACE(L"stack[%d] : userdata", idx);
			break;
		case OT_CLOSURE:
			TRACE(L"stack[%d] : closure", idx);
			break;
		case OT_NATIVECLOSURE:
			TRACE(L"stack[%d] : nativeclosure", idx);
			break;
		case OT_GENERATOR:
			TRACE(L"stack[%d] : generator", idx);
			break;
		case OT_USERPOINTER:
			TRACE(L"stack[%d] : userpointer", idx);
			break;
		case OT_THREAD:
			TRACE(L"stack[%d] : thread", idx);
			break;
		case OT_FUNCPROTO:
			TRACE(L"stack[%d] : funcproto", idx);
			break;
		case OT_CLASS:
			TRACE(L"stack[%d] : class", idx);
			break;
		case OT_INSTANCE:
			TRACE(L"stack[%d] : instance", idx);
			break;
		case OT_WEAKREF:
			TRACE(L"stack[%d] : weakref", idx);
			break;
		}
	}
}

#endif
