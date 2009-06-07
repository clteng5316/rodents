// sq.hpp
#pragma once

#include "preprocessor.hpp"
#include "meta.hpp"

#define SQ_ASSERT(expr)		ASSERT(SQ_SUCCEEDED(expr))
#define SQ_VERIFY(expr)		VERIFY(SQ_SUCCEEDED(expr))

namespace sq
{
	typedef SQInteger (*Function)(VM);

	//================================================================================

	class Error : public SQError
	{
	public:
		Error() throw();
		Error(PCWSTR format, ...) throw();
		Error(HRESULT hr, PCWSTR format, ...) throw();
	};

	SQInteger	raise(HSQUIRRELVM v, HRESULT hr, PCWSTR format, ...) throw();
	SQInteger	raise_va(HSQUIRRELVM v, HRESULT hr, PCWSTR format, va_list args) throw();
	void __declspec(noreturn) bad_cast(HSQUIRRELVM v, const type_info& from, const type_info& to) throw(Error);
	void		newclass(HSQUIRRELVM v, const type_info& type, const type_info& base, SQFUNCTION constructor) throw();
	SQInteger	newinstance(HSQUIRRELVM v, object* self) throw();

	inline SQInteger abs(HSQUIRRELVM v, SQInteger idx) throw()
	{
		return idx > 0 ? idx : sq_gettop(v) + idx + 1;
	}

	//================================================================================
	// get

	void get(HSQUIRRELVM v, SQInteger idx, HSQOBJECT* value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, bool*      value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, SQInteger* value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, BYTE*      value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, UINT*      value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, LONG*      value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, POINT*     value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, SIZE*      value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, PCWSTR*    value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, object**   value) throw(Error);
	void get(HSQUIRRELVM v, SQInteger idx, REFINTF    value) throw(Error);

	template < typename First, typename Second >
	void get(HSQUIRRELVM v, SQInteger idx, std::pair<First, Second>* value) throw(Error);

	template < typename T >
	void get(HSQUIRRELVM v, SQInteger idx, T** value) throw(Error);

	template < typename T >
	inline T get(HSQUIRRELVM v, SQInteger idx) throw(Error)
	{
		T value;
		get(v, idx, &value);
		return value;
	}

	template < typename T >
	inline T get_if_exists(HSQUIRRELVM v, SQInteger idx, const T& defval = T()) throw(Error)
	{
		return sq_gettype(v, idx) == OT_NULL ? defval : get<T>(v, idx);
	}

	//================================================================================
	// push

	void push(HSQUIRRELVM v, const Null&      value) throw();
	void push(HSQUIRRELVM v, HSQOBJECT        value) throw();
	void push(HSQUIRRELVM v, SQFUNCTION       value, SQUnsignedInteger params = 0) throw();
	void push(HSQUIRRELVM v, Function         value, SQUnsignedInteger params = 0) throw();
	void push(HSQUIRRELVM v, bool             value) throw();
	void push(HSQUIRRELVM v, SQInteger        value) throw();
	void push(HSQUIRRELVM v, UINT             value) throw();
	void push(HSQUIRRELVM v, DWORD            value) throw();
	void push(HSQUIRRELVM v, POINT            value) throw();
	void push(HSQUIRRELVM v, SIZE             value) throw();
	void push(HSQUIRRELVM v, POINT            value) throw();
	void push(HSQUIRRELVM v, PCSTR            value, SQInteger len = -1) throw();
	void push(HSQUIRRELVM v, PCWSTR           value, SQInteger len = -1) throw();
	void push(HSQUIRRELVM v, HWND             value) throw();
	void push(HSQUIRRELVM v, IImageList*      value) throw();
	void push(HSQUIRRELVM v, IShellItem*      value) throw();
	void push(HSQUIRRELVM v, IShellItemArray* value) throw();
	void push(HSQUIRRELVM v, IUnknown* value, HSQOBJECT* vtbl, sq::Function def) throw();

	//================================================================================

	template < typename T >
	void def(HSQUIRRELVM v, PCWSTR name, const T& value);

	//================================================================================

	class stackobj;

	class VM
	{
	private:
		HSQUIRRELVM		v;

	public:
		VM(HSQUIRRELVM vm = null) : v(vm)		{ }
		operator HSQUIRRELVM () const throw()	{ return v; }

		SQInteger	gettop() throw()				{ return sq_gettop(v); }
		void		settop(SQInteger top) throw()	{ sq_settop(v, top); }
		void		pop(SQInteger n) throw()		{ sq_pop(v, n); }

		template < typename T > T get(SQInteger idx) throw(Error);
		template < typename T > T get_if_exists(SQInteger idx, const T& defval = T()) throw(Error);
		template < typename T > void push(const T& value);
		template < typename T > void def(PCWSTR name, const T& value);

		stackobj getslot(SQInteger idx, PCWSTR key) throw(Error);
		void newslot(SQInteger idx) throw(Error);
		template < typename K, typename V > void newslot(SQInteger idx, const K& key, const V& value) throw(Error);

		inline void newtable() throw()		{ sq_newtable(v); }
		inline void newarray() throw()		{ sq_newarray(v, 0); }

		template < typename T >	void newarray(const T value[], size_t n);
		template < typename T >	void getarray(SQInteger idx, T value[], size_t n);

		template < typename T                > void newclass();
		template < typename T, typename Base > void newclass();

		SQInteger	abs(SQInteger idx) throw();
		void		append(SQInteger idx) throw();
		void		close() throw();
		PCWSTR		tostring(SQInteger idx);
	};

	class stackobj
	{
	private:
		VM			v;
		SQInteger	idx;
	public:
		stackobj(VM v, SQInteger idx) : v(v), idx(v.abs(idx)) {}
		template < typename T > operator T () throw(Error) { return v.get<T>(idx); }
	};

	class iterator
	{
	private:
		sq::VM		m_v;
		SQInteger	m_top;
		SQInteger	m_idx;

	public:
		iterator(sq::VM v, SQInteger idx) throw();
		~iterator() throw();
		bool next() throw();
	};

#define foreach_0(v, idx, iter) \
	for (sq::iterator iter((v), (idx)); iter.next();)
#define foreach(v, idx) \
	foreach_0((v), (idx), PP_ID(_sq_iter_))

	// Global VM
	extern VM			vm;

	//================================================================================

	HSQUIRRELVM	open() throw();
	void		call(HSQUIRRELVM v, SQInteger args, SQBool ret);
	SQInteger	import(HSQUIRRELVM v, PCWSTR filename);
	SQInteger	getslot(HSQUIRRELVM v);

	inline void rawset(HSQUIRRELVM v, SQInteger idx)
	{
		SQ_VERIFY( sq_rawset(v, idx) );
	}

	inline void newclass(HSQUIRRELVM v, SQBool hasbase = SQTrue)
	{
		SQ_VERIFY( sq_newclass(v, hasbase) );
	}
}

#include "sq.inl"

extern SQInteger def_config(sq::VM v);
extern SQInteger def_io(sq::VM v);
extern SQInteger def_os(sq::VM v);
extern SQInteger def_ui(sq::VM v);

#define def_member(name)	v.def(_SC(#name), null)
#define def_method(name)	v.def(_SC(#name), &CLASS::name)
