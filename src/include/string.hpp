// string.hpp
#pragma once

#include "autoptr.hpp"

extern bool	MatchContainsI(PCWSTR pattern, PCWSTR text) throw();
extern bool	MatchPatternI(PCWSTR pattern, PCWSTR text) throw();

// ï∂éöóÒÇÃî‰ärÇÕëÂï∂éöè¨ï∂éöÇãÊï ÇµÇ»Ç¢Ç±Ç∆Ç…íçà”
struct Less
{
	bool operator () (PCWSTR lhs, PCWSTR rhs) const throw()
	{
		return _wcsicmp(lhs, rhs) < 0;
	}

	template < typename T, typename U >
	bool operator () (const std::pair<T, U>& lhs, const std::pair<T, U>& rhs) const throw()
	{
		return operator () (lhs.first, rhs);
	}

	template < typename T, typename U >
	bool operator () (const std::pair<T, U>& lhs, const T& rhs) const throw()
	{
		return operator () (lhs.first, rhs);
	}

	template < typename T, typename U >
	bool operator () (const T& lhs, const std::pair<T, U>& rhs) const throw()
	{
		return operator () (lhs, rhs.first);
	}
};

//========================================================================================================================

namespace str
{
	inline bool		empty(PCSTR s) throw()		{ return !s || !s[0]; }
	inline bool		empty(PCWSTR s) throw()		{ return !s || !s[0]; }
	inline string	dup(PCWSTR s) throw()		{ return string(_wcsdup(s)); }

	template < typename T >
	inline AutoPtr<WCHAR, T> dup(PCWSTR s) throw()
	{
		if (!s)
			return null;
		size_t len = wcslen(s);
		PWSTR clone = (PWSTR) T::Alloc((len + 1) * sizeof(WCHAR));
		memcpy(clone, s, len * sizeof(WCHAR));
		clone[len] = L'\0';
		return AutoPtr<WCHAR, T>(clone);
	}

	//	inline bool		eq(PCWSTR lhs, PCWSTR rhs)
	extern void		copy_trim(PWSTR dst, size_t dstlen, PCWSTR src);

	inline PCWSTR load(UINT nID, WCHAR outString[], size_t cch)
	{
		LoadString(NULL, nID, outString, (int)cch);
		return outString;
	}

	template < size_t sz >
	inline PCWSTR load(UINT nID, WCHAR (&outString)[sz])
	{
		return load(nID, outString, sz);
	}
}

//========================================================================================================================

enum Encoding
{
	ENCODING_UNKNOWN,
	ENCODING_ASCII,
	ENCODING_SJIS,
	ENCODING_EUCJP,
	ENCODING_JIS,
	ENCODING_UTF7,
	ENCODING_UTF8,
	ENCODING_UTF8_BOM,
	ENCODING_UTF16_LE,
	ENCODING_UTF16_BE,
};

#define CP_SJIS		932
#define CP_EUCJP	20932
#define CP_JIS		50220

extern Encoding GetEncoding(PCSTR str, size_t len);

//========================================================================================================================

class _S
{
private:
	WCHAR	m_buffer[64];
public:
	_S(int nID)
	{
		m_buffer[0] = 0;
		::LoadString(NULL, nID, m_buffer, lengthof(m_buffer));
	}
	operator PCWSTR () const throw()
	{
		return m_buffer;
	}
};

//==========================================================================================================================

class StringBuffer
{
private:
	PWSTR	m_buffer;
	size_t	m_end;
	size_t	m_max;

public:
	StringBuffer() throw() : m_buffer(null), m_end(0), m_max(0) {}
	~StringBuffer()	{ free(m_buffer); }

	PWSTR	data() throw()				{ return empty() ? 0 : m_buffer; }
	PCWSTR	data() const throw()		{ return empty() ? 0 : m_buffer; }
	operator PWSTR () throw()			{ return data(); }
	operator PCWSTR () const throw()	{ return data(); }
	bool	empty() const throw()		{ return m_end == 0; }
	size_t	size() const throw()		{ return m_end; }
	void	clear() throw()				{ m_end = 0; }

	void append(WCHAR c) throw();
	void append(PCWSTR data, size_t ln) throw();
	void append(PCWSTR data) throw();
	void format(PCWSTR fmt, ...) throw();
	void resize(size_t sz) throw();
	void reserve(size_t capacity) throw();
	void terminate() throw()	{ append(L'\0'); }

private:
	StringBuffer(const StringBuffer& rhs) throw();
	StringBuffer& operator = (const StringBuffer& rhs) throw();
};

//==========================================================================================================================

class RingBuffer
{
private:
	PSTR	m_buffer;
	size_t	m_begin;
	size_t	m_end;
	size_t	m_max;

public:
	RingBuffer();
	~RingBuffer();
	PSTR	resize(size_t len);
	void	shift(size_t len);

	PSTR	data() throw()				{ return empty() ? 0 : m_buffer + m_begin; }
	PCSTR	data() const throw()		{ return empty() ? 0 : m_buffer + m_begin; }
	operator PSTR () throw()			{ return data(); }
	operator PCSTR () const throw()		{ return data(); }
	bool	empty() const throw()		{ return m_end - m_begin == 0; }
	size_t	size() const throw()		{ return m_end - m_begin; }
	void	clear() throw()				{ m_begin = m_end = 0; }

private:
	RingBuffer(const RingBuffer& rhs) throw();
	RingBuffer& operator = (const RingBuffer& rhs) throw();
};
