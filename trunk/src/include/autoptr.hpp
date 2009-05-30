// autoptr.hpp
#pragma once

struct CrtMem
{
	static void*	Alloc(size_t sz)			{ return malloc(sz); }
	static void*	Realloc(void* p, size_t sz)	{ return realloc(p, sz); }
	static void		Free(void* p)				{ free(p); }
};

struct CoTaskMem
{
	static void*	Alloc(size_t sz)			{ return ::CoTaskMemAlloc(sz); }
	static void*	Realloc(void* p, size_t sz)	{ return ::CoTaskMemRealloc(p, sz); }
	static void		Free(void* p)				{ ::CoTaskMemFree(p); }
};

template < typename T, typename Mem = CrtMem >
class AutoPtr
{
	struct Proxy
	{
		T**	m_ptr;
		explicit Proxy(T** ptr) : m_ptr(ptr)	{}
	};

private:
	T*	m_ptr;

public:
	AutoPtr(const Null&) : m_ptr(0)					{}
	explicit AutoPtr(T* ptr = null) : m_ptr(ptr)	{}
	explicit AutoPtr(size_t sz) : m_ptr((T*)Mem::Alloc(sz))	{}
	~AutoPtr()	{ Mem::Free(m_ptr); }

	AutoPtr(AutoPtr& rhs) throw()
	{
		m_ptr = rhs.detach();
	}

	AutoPtr& operator = (AutoPtr& rhs) throw()
	{
		attach(rhs.detach());
		return *this;
	}

	AutoPtr(Proxy rhs) throw()
	{
		m_ptr = *rhs.m_ptr;
		*rhs.m_ptr = NULL;
	}

	AutoPtr& operator = (Proxy rhs) throw()
	{
		attach(*rhs.m_ptr);
		*rhs.m_ptr = NULL;
		return *this;
	}

	operator Proxy() throw()
	{
		return Proxy(&m_ptr);
	}

	operator const T* () const throw()	{ return m_ptr; }
	operator       T* ()       throw()	{ return m_ptr; }
	T* operator -> () const throw()		{ ASSERT(m_ptr); return m_ptr; }
	T& operator * () const throw()		{ ASSERT(m_ptr); return *m_ptr; }
	T** operator & () throw()			{ ASSERT(!m_ptr); return &m_ptr; }

	T* get_ptr() const throw()		{ return m_ptr; }
	__declspec(property(get=get_ptr)) T* ptr;

	void clear() throw()
	{
		Mem::Free(m_ptr);
		m_ptr = null;
	}

	void attach(T* ptr) throw()
	{
		if (m_ptr != ptr)
		{
			Mem::Free(m_ptr);
			m_ptr = ptr;
		}
	}

	T* detach() throw()
	{
		T* tmp = m_ptr;
		m_ptr = null;
		return tmp;
	}

	void realloc(size_t sz) throw()
	{
		m_ptr = (T*)Mem::Realloc(m_ptr, sz);
	}

	void resize(size_t sz) throw()
	{
		realloc(sizeof(T) * sz);
	}
};

typedef AutoPtr<WCHAR>				string;
typedef AutoPtr<WCHAR, CoTaskMem>	CoStr;
