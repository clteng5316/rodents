// sq.cpp
#include "stdafx.h"
#include "sq.hpp"
#include "string.hpp"
#include "ref.hpp"
#include "win32.hpp"
#include "resource.h"

sq::VM	sq::vm;

static const char* getname(const type_info& type)
{
	PCSTR	name = type.name();
	if (strncmp(name, "class ", 6) == 0)
		return name + 6;
	else
		return name;
}

//================================================================================
// class object

#define MAX_IDENTLEN		MAX_PATH
#define object_get_prefix	L"get_";
#define object_set_prefix	L"set_";
#define object_tag			((void*)(&typeid(object)))
#define IUnknown_tag		((void*)(&IID_IUnknown))

// obj.attr という呼び出しを obj.getdelegate().get_attr() に転送する。
static SQInteger delegate_get(HSQUIRRELVM v)
{
	PCWSTR	name;

	if SQ_SUCCEEDED(sq_getstring(v, 2, &name))
	{
		WCHAR	attr[MAX_IDENTLEN] = object_get_prefix;
		wcscat_s(attr, name);
		sq_getdelegate(v, 1);
		sq_pushstring(v, attr, -1);
		if SQ_SUCCEEDED(sq_rawget(v, -2))
		{
			sq_push(v, 1);	// this
			sq::call(v, 1, SQTrue);
			return 1;
		}
	}
	return SQ_ERROR;
}

// obj.attr = value という呼び出しを obj.getdelegate().set_attr(value) に転送する。
static SQInteger delegate_set(HSQUIRRELVM v)
{
	PCWSTR	name;

	if SQ_SUCCEEDED(sq_getstring(v, 2, &name))
	{
		WCHAR	attr[MAX_IDENTLEN] = object_set_prefix;
		wcscat_s(attr, name);
		sq_getdelegate(v, 1);
		sq_pushstring(v, attr, -1);
		if SQ_SUCCEEDED(sq_rawget(v, -2))
		{
			sq_push(v, 1);	// this
			sq_push(v, 3);	// value
			sq::call(v, 2, SQFalse);
			return 0;
		}
	}
	return SQ_ERROR;
}

// obj.attr という呼び出しを obj.get_attr() に転送する。
static SQInteger object_get(HSQUIRRELVM v)
{
	PCWSTR	name;

	if SQ_SUCCEEDED(sq_getstring(v, 2, &name))
	{
		WCHAR	attr[MAX_IDENTLEN] = object_get_prefix;
		wcscat_s(attr, name);
		sq_pushstring(v, attr, -1);
		if SQ_SUCCEEDED(sq_rawget(v, 1))
		{
			sq_push(v, 1);	// this
			sq::call(v, 1, SQTrue);
			return 1;
		}
	}
	return SQ_ERROR;
}

// obj.attr = value という呼び出しを obj.set_attr(value) に転送する。
static SQInteger object_set(HSQUIRRELVM v)
{
	PCWSTR	name;

	if SQ_SUCCEEDED(sq_getstring(v, 2, &name))
	{
		WCHAR	attr[MAX_IDENTLEN] = object_set_prefix;
		wcscat_s(attr, name);
		sq_pushstring(v, attr, -1);
		if SQ_SUCCEEDED(sq_rawget(v, 1))
		{
			sq_push(v, 1);	// this
			sq_push(v, 3);	// value
			sq::call(v, 2, SQFalse);
			return 0;
		}
	}
	return SQ_ERROR;
}

static SQInteger object_finalize(SQUserPointer p, SQInteger size)
{
	static_cast<object*>(p)->finalize();
	return 1;
}

SQInteger sq::newinstance(HSQUIRRELVM v, object* self) throw()
{
	sq_setinstanceup(v, 1, self);
	try
	{
		self->constructor(v);
		sq_setreleasehook(v, 1, object_finalize);
		return 0;
	}
	catch (Error&)
	{
		self->finalize();
		return SQ_ERROR;
	}
}

//================================================================================

sq::Error::Error() throw()
{
}

sq::Error::Error(PCWSTR format, ...) throw()
{
	va_list args;
	va_start(args, format);
	raise_va(sq::vm, S_OK, format, args);
	va_end(args);
}

sq::Error::Error(HRESULT hr, PCWSTR format, ...) throw()
{
	va_list args;
	va_start(args, format);
	raise_va(sq::vm, hr, format, args);
	va_end(args);
}

SQInteger sq::raise(HSQUIRRELVM v, HRESULT hr, PCWSTR format, ...) throw()
{
	va_list args;
	va_start(args, format);
	SQInteger r = raise_va(v, S_OK, format, args);
	va_end(args);
	return r;
}

SQInteger sq::raise_va(HSQUIRRELVM v, HRESULT hr, PCWSTR format, va_list args) throw()
{
	WCHAR	buffer[1024];
	WCHAR	message[1024];

	if (format)
		vswprintf_s(buffer, format, args);
	else
		buffer[0] = L'\0';

	if (FAILED(hr) && 0 < FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			null, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			message, lengthof(message), null))
	{
		size_t	len;
		len = wcslen(message);
		while (len > 0 && iswspace(message[len - 1])) { --len; }
		message[len] = '\0';
		len = wcslen(buffer);
		swprintf_s(buffer + len, lengthof(buffer) - len,
			L"\n(0x%08X: %s)", hr, message);
	}

	return sq_throwerror(v, buffer);
}

void sq::bad_cast(HSQUIRRELVM v, const type_info& from, const type_info& to)
{
	throw Error(E_INVALIDARG, L"bad cast: from %S to %S", getname(from), getname(to));
}

static PCWSTR simplify_src(PCWSTR src)
{
	if (!src)
		return L"";
	size_t len = wcslen(PATH_SCRIPT);
	if (wcsncmp(src, PATH_SCRIPT, len) == 0 && src[len] == L'\\')
		src += len + 1;
	return src;
}

static StringBuffer	error_message;
static StringBuffer	error_details;

static SQInteger on_error(HSQUIRRELVM v)
{
	PCWSTR			err;

	if (error_message.empty() && sq_gettop(v) >= 1)
	{
		// メッセージ
		if SQ_FAILED(sq_getstring(v, 2, &err))
			err = L"(unknown)";
		error_message.append(err);

		// 詳細 (コールスタック)
		error_details.append(_S(STR_CALLSTACK));
		error_details.append(L"\n");
		SQStackInfos	si;
		int				depth = 0;
		for (SQInteger i = 1; SQ_SUCCEEDED(sq_stackinfos(v, i, &si)); ++i)
		{
			PCWSTR	func = si.funcname;
			PCWSTR	suffix = L"()";
			if (str::empty(si.funcname))
			{
				func = L"(anonymous)";
				suffix = L"";
			}
			if (si.line >= 0 && si.source)
				error_details.format(L"[%d] %s%s in %s (%d)\n", ++depth, func, suffix, simplify_src(si.source), si.line);
			else if (wcscmp(func, L"unknown") != 0)
				error_details.format(L"[%d] %s%s in C\n", ++depth, func, suffix);
		}
	}
	return 0;
}

static void on_compile_error(HSQUIRRELVM v, PCWSTR err, PCWSTR src, SQInteger line, SQInteger column)
{
	// メッセージ
	error_message.append(err);

	// 詳細 (ソース位置)
	src = simplify_src(src);
	error_details.format(_S(STR_SRCFORMAT), src, line, column);
}

//================================================================================

static void print(HSQUIRRELVM v, PCWSTR s, ...)
{
	WCHAR	buffer[1024];

	va_list args;
	va_start(args, s);
	vswprintf_s(buffer, s, args);
	va_end(args);

	sq_pushroottable(v);
	sq_pushstring(v, L"stdout", -1);
	if SQ_SUCCEEDED(sq_get(v, -2))
	{
		sq_pushroottable(v);
		sq_pushstring(v, buffer, -1);
		sq_call(v, 2, SQFalse, SQFalse);
	}
	sq_reseterror(v);
}

#ifdef _DEBUG
static void debug_print(HSQUIRRELVM v, PCWSTR s, ...)
{
	va_list args;
	va_start(args, s);
	TRACE_V(s, args);
	va_end(args);
}
#else
#	define debug_print	NULL
#endif

HSQUIRRELVM sq::open()
{
	ASSERT(!sq::vm);

	sq::VM v = sq_open(1024);
	sq::vm = v;

	sq_setprintfunc(v, print, debug_print);

	sq_pushroottable(v);

	sqstd_register_mathlib(v);
	sqstd_register_stringlib(v);

	// ARGS
	int		argc = 0;
	PWSTR*	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	v.push(L"ARGS");
	v.newarray(argv, argc);
	v.newslot(-3);

	// class object
	v.push(L"object");
	sq::newclass(v, SQFalse);
	sq_settypetag(v, -1, object_tag);
	v.def(L"_get", object_get);
	v.def(L"_set", object_set);
	v.newslot(-3);

	v.pop(1);	// pop root table

	// Error handlers
	sq_setcompilererrorhandler(v, on_compile_error);
	sq_newclosure(v, on_error, 0);
	sq_seterrorhandler(v);

	return v;
}

void sq::call(HSQUIRRELVM v, SQInteger args, SQBool ret)
{
	SQInteger top = sq_gettop(v);

	if SQ_SUCCEEDED(sq_call(v, args, ret, SQTrue))
		return;

	if (error_message.empty())
	{
		sq_getlasterror(v);
		SQObjectType type = sq_gettype(v, -1);
		if (type != OT_NULL)
		{
			if (type != OT_STRING)
				sq_tostring(v, -1);
			PCWSTR message;
			sq_getstring(v, -1, &message);
			error_message.append(message);
		}
	}

	error_message.terminate();
	error_details.terminate();
	Alert(error_message, error_details, _S(STR_SCRIPTERROR), MB_ICONERROR);
	error_message.clear();
	error_details.clear();
	sq_reseterror(v);

	sq_settop(v, top);
	if (ret)
		sq_pushnull(v);
}

//================================================================================

// gettop(table, key1, key2, ...) returns table[key1][key2][...]
SQInteger sq::getslot(HSQUIRRELVM v)
{
	SQInteger top = sq_gettop(v);
	SQInteger tbl = 2;
	for (SQInteger i = 3; i <= top; ++i)
	{
		sq_push(v, i);
		if SQ_FAILED(sq_get(v, tbl))
		{
			sq_reseterror(v);
			sq_pushnull(v);
			break;
		}
		tbl = sq_gettop(v);
	}
	return 1;
}

void sq::get(HSQUIRRELVM v, SQInteger idx, POINT* value) throw(Error)
{
	idx = sq::abs(v, idx);
	push(v, 0);
	sq_get(v, idx);
	get(v, -1, &value->x);
	push(v, 1);
	sq_get(v, idx);
	get(v, -1, &value->y);
}

void sq::get(HSQUIRRELVM v, SQInteger idx, SIZE* value) throw(Error)
{
	sq::get(v, idx, (POINT*)value);
}

void sq::get(HSQUIRRELVM v, SQInteger idx, PCWSTR* value) throw(Error)
{
	if (sq_gettype(v, idx) == OT_NULL)
		*value = null;
	else if (SQ_FAILED(sq_getstring(v, idx, value)))
		throw Error();
}

void sq::get(HSQUIRRELVM v, SQInteger idx, object** value)
{
	ASSERT(value);
	SQUserPointer self;
	if (SQ_FAILED(sq_getinstanceup(v, idx, &self, object_tag)))
		throw Error();
	*value = static_cast<object*>(self);
}

void sq::get(HSQUIRRELVM v, SQInteger idx, REFINTF value)
{
	HRESULT			hr = S_OK;
	SQUserPointer	self;
	SQUserPointer	tag;
	if (SQ_FAILED(sq_getuserdata(v, idx, &self, &tag)) ||
		tag != IUnknown_tag ||
		FAILED(hr = (*(IUnknown**)self)->QueryInterface(value.iid, value.pp)))
	{
		throw Error(hr, L"IUnknown.QueryInterface() failed (type=0x%08X)", sq_gettype(v, idx));
	}

	// QueryInterface で参照カウントが増えすぎているので、再び減らす。
	// FIXME: Unknown がプロキシ型で実装されていると問題になる可能性がある。
	(*(IUnknown**)value.pp)->Release();
}

void sq::push(HSQUIRRELVM v, PCSTR value, SQInteger len) throw()
{
	if (len < 0)
		len = strlen(value);
	if (len <= 0)
		sq_pushnull(v);
	else
	{
		string str(sizeof(WCHAR) * len);
		len = MultiByteToWideChar(CP_ACP, 0, value, (int)len, str, (int)len);
		sq_pushstring(v, str, len);
	}
}

void sq::push(HSQUIRRELVM v, POINT value) throw()
{
	// [ x, y ]
	sq_newarray(v, 2);
	sq_pushinteger(v, 0);
	sq_pushinteger(v, value.x);
	sq::rawset(v, -3);
	sq_pushinteger(v, 1);
	sq_pushinteger(v, value.y);
	sq::rawset(v, -3);
}

static SQInteger IUnknown_Release(SQUserPointer p, SQInteger size)
{
	if (IUnknown** pp = (IUnknown**)p)
		(*pp)->Release();
	return 1;
}

void sq::push(HSQUIRRELVM v, IUnknown* value, HSQOBJECT* vtbl, sq::Function def) throw()
{
	if (value)
	{
		IUnknown** pp = (IUnknown**)sq_newuserdata(v, sizeof(IUnknown*));
		*pp = value;
		sq_setreleasehook(v, -1, IUnknown_Release);
		sq_settypetag(v, -1, IUnknown_tag);
		value->AddRef();

		if (vtbl)
		{
			if (!sq_istable(*vtbl))
			{
				sq_newtable(v);
				sq::def(v, L"_get", delegate_get);
				sq::def(v, L"_set", delegate_set);
				def(v);
				sq_getstackobj(v, -1, vtbl);
				sq_addref(v, vtbl);
				sq_pop(v, 1);
			}
			sq::push(v, *vtbl);
			sq_setdelegate(v, -2);
		}
	}
	else
		sq_pushnull(v);
}

// The base class is retrieved from top of stack.
void sq::newclass(HSQUIRRELVM v, const type_info& type, const type_info& base, SQFUNCTION constructor) throw()
{
	ASSERT(typeid(object) <= base);	// base must be an object or subclass
	// FIXME: なぜか RenameDialog, Dialog でうまく動かないので一時的に無効化している。
//	ASSERT(base < type);			// type must inherit base

	push(v, getname(type));
	push(v, getname(base));
	SQ_VERIFY( sq_get(v, -3) );
	sq::newclass(v, SQTrue);

	sq_settypetag(v, -1, object_tag);

	// constructor()
	push(v, L"constructor");
	sq_newclosure(v, constructor, 0);
	sq::rawset(v, -3);
}

//================================================================================

void sq::VM::close() throw()
{
	if (v)
	{
		sq_close(v);
		v = null;
	}
}

sq::stackobj sq::VM::getslot(SQInteger idx, PCWSTR key) throw(Error)
{
	idx = sq::abs(v, idx);
	push(key);
	if SQ_FAILED(sq_get(v, idx))
		throw sq::Error(L"key \"%s\" not found", key);
	return sq::stackobj(*this, -1);
}

PCWSTR sq::VM::tostring(SQInteger idx)
{
	PCWSTR r;
	sq_tostring(v, idx);
	sq_getstring(v, -1, &r);
	return r;
}

//================================================================================

sq::iterator::iterator(sq::VM v, SQInteger idx) : m_v(v)
{
	m_idx = v.abs(idx);
	m_v.push(null);
	m_top = m_v.gettop();
}

sq::iterator::~iterator()
{
	sq_settop(m_v, m_top - 1);
}

bool sq::iterator::next() throw()
{
	sq_settop(m_v, m_top);
	return SQ_SUCCEEDED(sq_next(m_v, m_idx));
}
