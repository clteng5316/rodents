// sq.inl
#pragma once

//================================================================================
// get

inline void sq::get(HSQUIRRELVM v, SQInteger idx, bool* value) throw(Error)
{
	SQBool b;
	if (SQ_FAILED(sq_getbool(v, idx, &b)))
		throw Error();
	*value = (b == SQTrue);
}

inline void sq::get(HSQUIRRELVM v, SQInteger idx, SQInteger* value) throw(Error)
{
	if (SQ_FAILED(sq_getinteger(v, idx, value)))
		throw Error();
}

inline void sq::get(HSQUIRRELVM v, SQInteger idx, BYTE* value) throw(Error)
{
	SQInteger	i;
	sq::get(v, idx, &i);
	*value = (BYTE)i;
}

inline void sq::get(HSQUIRRELVM v, SQInteger idx, UINT* value) throw(Error)
{
	STATIC_ASSERT( sizeof(UINT) == sizeof(SQInteger) );
	sq::get(v, idx, (SQInteger*)value);
}

inline void sq::get(HSQUIRRELVM v, SQInteger idx, LONG* value) throw(Error)
{
	STATIC_ASSERT( sizeof(UINT) == sizeof(SQInteger) );
	sq::get(v, idx, (SQInteger*)value);
}

inline void sq::get(HSQUIRRELVM v, SQInteger idx, HSQOBJECT* value) throw(Error)
{
	if (SQ_FAILED(sq_getstackobj(v, idx, value)))
		throw Error();
}

template < typename TImplements, typename TMixin > class Unknown;

namespace sq
{
	namespace detail
	{
		// object の派生クラス用
		template < bool isUnknown >
		struct Getter
		{
			template < typename T >
			inline static void get(HSQUIRRELVM v, SQInteger idx, T** value)
			{
				object* obj;
				sq::get(v, idx, &obj);
				if (T* self = dynamic_cast<T*>(obj))
					*value = self;
				else
					bad_cast(v, typeid(*obj), typeid(T));
			}
		};

		// IUnknown の派生クラス用
		template <>
		struct Getter<true>
		{
			template < typename T >
			inline static void get(HSQUIRRELVM v, SQInteger idx, T** value)
			{
				sq::get(v, idx, REFINTF(value));
			}
		};

		template < typename T >
		struct IsUnknown
		{
			struct U1 {	char dummy[1]; };
			struct U2 {	char dummy[2]; };
			static U1 test(IUnknown*);
			template < typename imp, typename mix >
			static U1 test(Unknown<imp, mix>*);
			static U2 test(...);
			static T* make();
		public:
			enum { value = (sizeof(U1) == sizeof(test(make()))) };
		};
	}
}

template < typename T >
inline void sq::get(HSQUIRRELVM v, SQInteger idx, T** value) throw(Error)
{
	// object 派生ではなく、かつ IUnknown 派生である場合のみ、IUnknown 版を呼び出す。
	detail::Getter<
		!meta::Convertable<T*, object*>::value && detail::IsUnknown<T>::value
	>::get(v, idx, value);
}

//================================================================================
// push

inline void sq::push(HSQUIRRELVM v, const Null&) throw()
{
	sq_pushnull(v);
}

inline void sq::push(HSQUIRRELVM v, HSQOBJECT value) throw()
{
	sq_pushobject(v, value);
}

inline void sq::push(HSQUIRRELVM v, SQFUNCTION value, SQUnsignedInteger params) throw()
{
	sq_newclosure(v, value, params);
}

inline void sq::push(HSQUIRRELVM v, Function value, SQUnsignedInteger params) throw()
{
	sq_newclosure(v, (SQFUNCTION)value, params);
}

inline void sq::push(HSQUIRRELVM v, bool value) throw()
{
	sq_pushbool(v, value);
}

inline void sq::push(HSQUIRRELVM v, SQInteger value) throw()
{
	sq_pushinteger(v, value);
}

inline void sq::push(HSQUIRRELVM v, UINT value) throw()
{
	sq_pushinteger(v, (SQInteger)value);
}

inline void sq::push(HSQUIRRELVM v, DWORD value) throw()
{
	sq_pushinteger(v, (SQInteger)value);
}

inline void sq::push(HSQUIRRELVM v, SIZE value) throw()
{
	sq::push(v, (POINT&)value);
}

inline void sq::push(HSQUIRRELVM v, PCWSTR value, SQInteger len) throw()
{
	sq_pushstring(v, value, len);
}

//================================================================================

namespace sq
{
	//================================================================================
	// invoke

#define PP_CALL_FUNCTION(n, call_t)											\
	template < typename R PP_TYPENAME_APPEND(n) >							\
	inline SQInteger invoke(HSQUIRRELVM v,									\
		R (call_t *fn)(PP_ARGS(n)) PP_ARGS_APPEND(n))						\
	{																		\
		push(v, fn(PP_ARGV(n)));											\
		return 1;															\
	}																		\
	template < PP_TYPENAME(n) >												\
	inline SQInteger invoke(HSQUIRRELVM v,									\
		void (call_t *fn)(PP_ARGS(n)) PP_ARGS_APPEND(n))					\
	{																		\
		fn(PP_ARGV(n));														\
		return 0;															\
	}

#define PP_CALL_MEMBER(n, const_t)											\
	template < typename R, typename T, typename U PP_TYPENAME_APPEND(n) >	\
	inline SQInteger invoke(HSQUIRRELVM v, T* self,							\
		R (U::*fn)(PP_ARGS(n)) const_t PP_ARGS_APPEND(n))					\
	{																		\
		push(v, (self->*fn)(PP_ARGV(n)));									\
		return 1;															\
	}																		\
	template < typename T, typename U PP_TYPENAME_APPEND(n) >				\
	inline SQInteger invoke(HSQUIRRELVM v, T* self,							\
		void (U::*fn)(PP_ARGS(n)) const_t PP_ARGS_APPEND(n))				\
	{																		\
		(self->*fn)(PP_ARGV(n));											\
		return 0;															\
	}

#define PP_CALL(n)					\
	PP_CALL_FUNCTION(n, __cdecl)	\
	PP_CALL_FUNCTION(n, __stdcall)	\
	PP_CALL_MEMBER(n, PP_NULL)		\
	PP_CALL_MEMBER(n, const)

PP_STATEMENT(9, PP_CALL)

#undef PP_CALL
#undef PP_CALL_MEMBER

	//================================================================================
	// def

#define PP_GET(n)	A##n a##n = (n < argc ? get<A##n>(v, 1+n) : A##n());

#define PP_DEF_ARGS(n)	\
	const SQInteger argc = sq_gettop(v) - 1;\
	PP_REPEAT(n, PP_GET, PP_NULL);

#define PP_DEF_FUNCTION(n, call_t)										\
	template < typename R PP_TYPENAME_APPEND(n) >						\
	static SQInteger F##n##call_t(HSQUIRRELVM v)						\
	{																	\
		try																\
		{																\
			R (call_t *fn)(PP_ARGS(n));									\
			sq_getuserpointer(v, -1, (SQUserPointer*)&fn);				\
			PP_DEF_ARGS(n);												\
			return invoke(v, fn PP_ARGV_APPEND(n));						\
		}																\
		catch (Error&) { return SQ_ERROR; }								\
	}																	\
	template < typename R PP_TYPENAME_APPEND(n) >						\
	void push(HSQUIRRELVM v, R (call_t *fn)(PP_ARGS(n)))				\
	{																	\
		sq_pushuserpointer(v, fn);										\
		sq_newclosure(v, &F##n##call_t< R PP_ARGT_APPEND(n) >, 1);		\
	}

#define PP_DEF_METHOD(n, const_t)										\
	template < typename R, typename T PP_TYPENAME_APPEND(n) >			\
	void push(HSQUIRRELVM v, R (T::*fn)(PP_ARGS(n)) const_t)			\
	{																	\
		typedef R (T::*F)(PP_ARGS(n)) const_t;							\
		struct L														\
		{																\
			static SQInteger Do(HSQUIRRELVM v)							\
			{															\
				try														\
				{														\
					F* fn;												\
					sq_getuserdata(v, -1, (SQUserPointer*)&fn, NULL);	\
					T* self = get<T*>(v, 1);							\
					PP_DEF_ARGS(n);										\
					return invoke(v, self, *fn PP_ARGV_APPEND(n));		\
				}														\
				catch (Error&) { return SQ_ERROR; }						\
			}															\
		};																\
		*(F*)sq_newuserdata(v, sizeof(F)) = fn;							\
		sq_newclosure(v, &L::Do, 1);									\
	}

#define PP_DEF(n)					\
	PP_DEF_FUNCTION(n, __cdecl)		\
	PP_DEF_FUNCTION(n, __stdcall)	\
	PP_DEF_METHOD(n, PP_NULL)		\
	PP_DEF_METHOD(n, const)

#pragma warning ( disable : 4189 )
PP_STATEMENT(9, PP_DEF)
#pragma warning ( default : 4189 )
#undef PP_DEF
#undef PP_DEF_METHOD
#undef PP_DEF_FUNCTION
#undef PP_DEF_ARGS
#undef PP_GET

	template < typename T >
	void push(HSQUIRRELVM v, SQInteger (T::*fn)(sq::VM))
	{
		typedef SQInteger (T::*F)(sq::VM);
		struct L
		{
			static SQInteger Do(HSQUIRRELVM v)
			{
				try
				{
					F* fn;
					sq_getuserdata(v, -1, (SQUserPointer*)&fn, NULL);
					T* self = get<T*>(v, 1);
					return (self->**fn)(v);
				}
				catch (Error&) { return SQ_ERROR; }
			}
		};
		*(F*)sq_newuserdata(v, sizeof(F)) = fn;
		sq_newclosure(v, &L::Do, 1);
	}
}

template < typename T >
void sq::def(HSQUIRRELVM v, PCWSTR name, const T& value)
{
	::sq::push(v, name);
	::sq::push(v, value);
	SQ_VERIFY( sq_newslot(v, -3, SQFalse) );
}

//================================================================================
// VM

template < typename T >
inline void sq::VM::newclass()
{
	return newclass<T, object>();
}

template < typename T, typename Base >
inline void sq::VM::newclass()
{
	struct L
	{
		static SQInteger ctor(HSQUIRRELVM v) throw()
		{
			return newinstance(v, new T());
		}
	};

	::sq::newclass(v, typeid(T), typeid(Base), &L::ctor);
}

template < typename T >
inline void sq::VM::newarray(const T value[], size_t n)
{
	sq_newarray(v, 0);
	for (size_t i = 0; i < n; ++i)
	{
		push(value[i]);
		sq_arrayappend(v, -2);
	}
}

template < typename T >
inline void sq::VM::getarray(SQInteger idx, T value[], size_t n)
{
	idx = abs(idx);
	for (size_t i = 0; i < n; ++i)
	{
		sq_pushinteger(v, i);
		if SQ_SUCCEEDED(sq_get(v, idx))
		{
			sq::get(v, -1, &value[i]);
			sq_pop(v, 1);
		}
	}
}

template < typename T >
inline T sq::VM::get(SQInteger idx) throw(Error)
{
	return ::sq::get<T>(v, idx);
}

template < typename T >
inline T sq::VM::get_if_exists(SQInteger idx, const T& defval) throw(Error)
{
	return ::sq::get_if_exists<T>(v, idx, defval);
}

template < typename T >
inline void sq::VM::push(const T& value)
{
	::sq::push(v, value);
}

template < typename T >
inline void sq::VM::def(PCWSTR name, const T& value)
{
	::sq::def(v, name, value);
}

inline void sq::VM::newslot(SQInteger idx)
{
	if (SQ_FAILED(sq_newslot(v, idx, SQFalse)))
		throw Error();
}

template < typename K, typename V >
inline void sq::VM::newslot(SQInteger idx, const K& key, const V& value) throw(Error)
{
	idx = abs(idx);
	push(key);
	push(value);
	newslot(idx);
}

inline SQInteger sq::VM::abs(SQInteger idx) throw()
{
	return sq::abs(v, idx);
}

inline void sq::VM::append(SQInteger idx) throw()
{
	SQ_VERIFY( sq_arrayappend(v, idx) );
}
