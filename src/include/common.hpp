// common.hpp
#pragma once

//========================================================================================================================
// check

#ifndef _MT
#	error マルチスレッド設定でビルドする必要があります。
#endif

#ifndef _CPPUNWIND
#	error C++例外処理を有効にする必要があります。
#endif

#ifndef _NATIVE_WCHAR_T_DEFINED
#	error wchar_t を組み込み型として扱う必要があります。
#endif

//========================================================================================================================
// warning

#pragma warning ( disable : 4100 ) // 引数は関数の本体部で 1 度も参照されません。
#pragma warning ( disable : 4127 ) // 条件式が定数です。
#pragma warning ( disable : 4201 ) // 非標準の拡張機能が使用されています : 無名の構造体または共用体です。
#pragma warning ( disable : 4290 ) // C++ の例外の指定は無視されます。関数が __declspec(nothrow) でないことのみ表示されます。
#pragma warning ( error   : 4800 ) // <type> : ブール値を 'true' または 'false' に強制的に設定します

//========================================================================================================================

#define WINVER			0x0600
#define _WIN32_WINNT	0x0600
#define _WIN32_IE		0x0700

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

//========================================================================================================================
// Windows headers

#include <crtdbg.h>
#include <windows.h>
#include <comdef.h>

//========================================================================================================================
// InterlockedXXX

extern "C"
{
   long  __cdecl _InterlockedIncrement(long volatile *addend);
   long  __cdecl _InterlockedDecrement(long volatile *addend);
   long  __cdecl _InterlockedCompareExchange(long* volatile dest, long exchange, long comp);
   long  __cdecl _InterlockedExchange(long* volatile target, long value);
   long  __cdecl _InterlockedExchangeAdd(long* volatile addend, long value);
}

#pragma intrinsic (_InterlockedCompareExchange)
#define InterlockedCompareExchange _InterlockedCompareExchange

#pragma intrinsic (_InterlockedExchange)
#define InterlockedExchange _InterlockedExchange 

#pragma intrinsic (_InterlockedExchangeAdd)
#define InterlockedExchangeAdd _InterlockedExchangeAdd

#pragma intrinsic (_InterlockedIncrement)
#define InterlockedIncrement _InterlockedIncrement

#pragma intrinsic (_InterlockedDecrement)
#define InterlockedDecrement _InterlockedDecrement

//========================================================================================================================
// C++ headers

#include <typeinfo>
#include <vector>

//========================================================================================================================

const struct Null
{
	template < typename T > operator T* () const throw()	{ return 0; }
} null;

typedef ptrdiff_t		ssize_t;

// 静的配列の長さ
#define lengthof(ar)		(sizeof(ar) / sizeof(ar[0]))
#define endof(ar)			((ar) + lengthof(ar))

#undef min
template < typename T > inline T min(T a, T b)	{ return a < b ? a : b; }
#undef max
template < typename T > inline T max(T a, T b)	{ return a > b ? a : b; }

template < typename T >
inline void RELEASE(T*& p)
{
	if (p)
	{
		p->Release();
		p = NULL;
	}
}

//================================================================================
// 2D struct

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)	((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)	((int)(short)HIWORD(lp))
#endif
#define GET_XY_LPARAM(lp)	GET_X_LPARAM(lp), GET_Y_LPARAM(lp)

//==========================================================================================================================
// POINT

#define POINT_XY(o)			(o).x, (o).y

inline bool operator == (const POINT& lhs, const POINT& rhs) throw()	{ return memcmp(&lhs, &rhs, sizeof(POINT)) == 0; }
inline bool operator != (const POINT& lhs, const POINT& rhs) throw()	{ return !(lhs == rhs); }

inline POINT operator + (const POINT& lhs, const POINT& rhs)
{
	POINT result = { lhs.x + rhs.x, lhs.y + rhs.y };
	return result;
}

inline POINT operator - (const POINT& lhs, const POINT& rhs)
{
	POINT result = { lhs.x - rhs.x, lhs.y - rhs.y };
	return result;
}

inline POINT POINT_FROM_LPARAM(LPARAM lParam)
{
	POINT pt = { GET_XY_LPARAM(lParam) };
	return pt;
}

//==========================================================================================================================
// SIZE

#define SIZE_W(o)			((o).cx)
#define SIZE_H(o)			((o).cy)
#define SIZE_WH(o)			(o).cx, (o).cy

inline bool operator == (const SIZE& lhs, const SIZE& rhs)		{ return lhs.cx == rhs.cx && lhs.cy == rhs.cy; }
inline bool operator != (const SIZE& lhs, const SIZE& rhs)		{ return !(lhs == rhs); }

inline SIZE operator + (const SIZE& lhs, const SIZE& rhs)
{
	SIZE result = { lhs.cx + rhs.cx, lhs.cy + rhs.cy };
	return result;
}

inline SIZE operator - (const SIZE& lhs, const SIZE& rhs)
{
	SIZE result = { lhs.cx - rhs.cx, lhs.cy - rhs.cy };
	return result;
}

//==========================================================================================================================
// RECT

#define RECT_X(o)			((o).left)
#define RECT_Y(o)			((o).top)
#define RECT_W(o)			((o).right - (o).left)
#define RECT_H(o)			((o).bottom - (o).top)
#define RECT_XY(o)			RECT_X(o), RECT_Y(o)
#define RECT_WH(o)			RECT_W(o), RECT_H(o)
#define RECT_XYWH(o)		RECT_XY(o), RECT_WH(o)
#define RECT_LTRB(o)		(o).left, (o).top, (o).right, (o).bottom

#define RECT_LOCATION(o)	((POINT&)(o))
inline POINT	RECT_NW(const RECT& o)		{ POINT pt = { (o).left, (o).top }; return pt; }
inline POINT	RECT_NE(const RECT& o)		{ POINT pt = { (o).right, (o).top }; return pt; }
inline POINT	RECT_SW(const RECT& o)		{ POINT pt = { (o).left, (o).bottom }; return pt; }
inline POINT	RECT_SE(const RECT& o)		{ POINT pt = { (o).right, (o).bottom }; return pt; }

inline SIZE RECT_SIZE(const RECT& o)
{
	SIZE sz = { RECT_WH(o) };
	return sz;
}

#define RECT_CENTER_X(o)	(((o).left + (o).right)/2) 
#define RECT_CENTER_Y(o)	(((o).top + (o).bottom)/2)
#define RECT_CENTER(o)		RECT_CENTER_X(o), RECT_CENTER_Y(o)

inline RECT RECT_FROM_LTRB(int l, int t, int r, int b)	{ RECT o = { l, t, r, b }; return o; }
inline RECT RECT_FROM_XYWH(int x, int y, int w, int h)	{ return RECT_FROM_LTRB(x, y, x+w, y+h); }

inline bool operator == (const RECT& lhs, const RECT& rhs) throw()	{ return memcmp(&lhs, &rhs, sizeof(RECT)) == 0; }
inline bool operator != (const RECT& lhs, const RECT& rhs) throw()	{ return !(lhs == rhs); }

//========================================================================================================================
// RTTI

inline bool operator < (const type_info& lhs, const type_info& rhs) throw()
{
	return rhs.before(lhs) != 0;
}

inline bool operator <= (const type_info& lhs, const type_info& rhs) throw()
{
	return lhs == rhs || lhs < rhs;
}

//========================================================================================================================

typedef struct SQVM*	HSQUIRRELVM;
class REFINTF;

namespace sq
{
	class VM;
}

class __declspec(novtable) object
{
public:
	object() throw()			{}

protected:
	virtual ~object() throw()	{}

public:
	virtual void constructor(sq::VM v) throw(...) = 0;
	virtual void finalize() throw()		{ delete this; }

private:
	object(const object&);
	object& operator = (const object&);
};

//========================================================================================================================
// global environment

extern WCHAR APPNAME[MAX_PATH];
extern WCHAR PATH_ROOT[MAX_PATH];
extern WCHAR PATH_SCRIPT[MAX_PATH];

//========================================================================================================================
// STATIC_ASSERT : コンパイル時のアサーション

template < bool x > struct STATIC_ASSERTION_FAILURE;
template <> struct STATIC_ASSERTION_FAILURE<true>{};
template < int x > struct STATIC_ASSERT_TEST{};
#define STATIC_ASSERT(exp)	typedef STATIC_ASSERT_TEST < sizeof(STATIC_ASSERTION_FAILURE<(bool)(exp)>) > __STATIC_ASSERT_TYPEDEF__

//========================================================================================================================
// DEBUG

#ifdef _DEBUG
#define RELEASE_OR_DEBUG(r, d)	d
#define ASSERT(expr)	if (expr); else { if (::IsDebuggerPresent()) __debugbreak(); else _ASSERTE(expr); }
#define VERIFY(expr)	ASSERT(expr)
void	TRACE(PCWSTR format, ...) throw();
void	TRACE_V(PCWSTR format, va_list args) throw();
void	TRACE_HWND(HWND hwnd) throw();
void	TRACE_STACK(HSQUIRRELVM v) throw();
#else
#define RELEASE_OR_DEBUG(r, d)	r
#define ASSERT(expr)	__noop
#define VERIFY(expr)	((void)(expr))
#define TRACE			__noop
#define TRACE_V			__noop
#define TRACE_HWND		__noop
#define TRACE_STACK		__noop
#endif
