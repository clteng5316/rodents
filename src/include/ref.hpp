// ref.hpp
#pragma once

//========================================================================================================================
// 型付けされたインタフェースポインタの参照.
class REFINTF
{
public:
	REFIID iid;
	void** const pp;

	template < class T > REFINTF(T** ppT)              : iid(__uuidof(T)), pp((void**)ppT)	{ ASSERT(pp); }
	template < class T > REFINTF(REFIID riid, T** ppT) : iid(riid), pp((void**)ppT)			{ ASSERT(pp); }
	                     REFINTF(const REFINTF& r)     : iid(r.iid), pp(r.pp)				{ ASSERT(pp); }

private:
	REFINTF& operator = (const REFINTF&);
};

//========================================================================================================================

HRESULT	CreateInstance(REFCLSID clsid, REFINTF pp);
bool	same(IUnknown* lhs, IUnknown* rhs) throw();

//========================================================================================================================
// ref<T>

template < typename T >
inline void incref(T* p)
{
	if (p)
		p->AddRef();
}

template < typename T >
inline void decref(T* p)
{
	if (p)
		p->Release();
}

template < typename T >
class ref
{
public:
	typedef T*	pointer;

private:
	pointer	m_ptr;

public:
	         ref()                throw() : m_ptr(null)		{}
	         ref(const ref& p)    throw() : m_ptr(p)		{ incref(m_ptr); }
	explicit ref(T* p)            throw() : m_ptr(p)		{ incref(m_ptr); }
	         ref(const Null&)     throw() : m_ptr(null)		{}

	template < typename U >
	ref(const ref<U>& p) throw() : m_ptr(p.ptr)		{ incref(m_ptr); }

	~ref() throw()	{ decref(m_ptr); }

    ref& operator = (const ref& p) throw()		{ assign(p); return *this; }
    ref& operator = (T* p) throw()				{ assign(p); return *this; }
    ref& operator = (const Null&) throw()		{ decref(m_ptr); m_ptr = null; return *this; }

	void assign(T* p) throw()
	{
		incref(p);
		attach(p);
	}

	void attach(T* p) throw()
	{
		decref(m_ptr);
		m_ptr = p;
	}

	T* detach() throw()
	{
		T* tmp = m_ptr;
		m_ptr = null;
		return tmp;
	}

	template < typename U >
	static ref from(U* o) throw()
	{
		ref r;
		r.m_ptr = static_cast<T*>(o);
		return r;
	}

	HRESULT copyto(REFIID iid, void** pp) throw()
	{
		if (m_ptr)
			return m_ptr->QueryInterface(iid, pp);
		*pp = null;
		return E_POINTER;
	}

	HRESULT copyto(REFINTF pp) throw()
	{
		return copyto(pp.iid, pp.pp);
	}

	HRESULT newobj(REFCLSID clsid) throw()
	{
		ASSERT(!m_ptr);
		return CreateInstance(clsid, &m_ptr);
	}

	operator T* () const throw()	{ return m_ptr; }
	T* operator -> () const throw()	{ ASSERT(m_ptr); return m_ptr; }
	T& operator * () const throw()	{ ASSERT(m_ptr); return *m_ptr; }
	T** operator & () throw()		{ ASSERT(!m_ptr); return &m_ptr; }

	T* get_ptr() const throw()		{ return m_ptr; }
	__declspec(property(get=get_ptr)) T* ptr;
};

template < typename T, typename U >
inline bool operator == (const ref<T>& lhs, const ref<U>& rhs) throw()	{ return same(lhs, rhs); }
template < typename T, typename U >
inline bool operator == (T* lhs, const ref<U>& rhs) throw()				{ return same(lhs, rhs); }
template < typename T, typename U >
inline bool operator == (const ref<T>& lhs, U* rhs) throw()				{ return same(lhs, rhs); }

template < typename T, typename U >
inline ref<T>& operator >> (ref<T>& r, U& v)		{ *r >> v; return r; }
template < typename T, typename U >
inline ref<T>& operator << (ref<T>& r, const U& v)	{ *r << v; return r; }
