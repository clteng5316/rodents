// string.cpp

#include "stdafx.h"
#include "string.hpp"

//=============================================================================

void str::copy_trim(PWSTR dst, size_t dstlen, PCWSTR src)
{
	while (*src && iswspace(*src)) { ++src; }
	wcscpy_s(dst, dstlen, src);
	for (size_t i = wcslen(dst); i > 0 && iswspace(dst[i - 1]); i--)
		dst[i - 1] = '\0';
}

//=============================================================================

namespace
{
	inline bool eq(WCHAR lhs, WCHAR rhs) throw()	{ return ChrCmpI(lhs, rhs) == 0; }
	inline PCWSTR inc(PCWSTR s) throw()				{ return s + 1; }
}

bool MatchContainsI(PCWSTR pattern, PCWSTR text) throw()
{
	return StrStrI(text, pattern) != null;
}

bool MatchPatternI(PCWSTR pattern, PCWSTR text) throw()
{
	PCWSTR cp = 0;
	PCWSTR mp = 0;
	PCWSTR text_ = text;

	while (*text_ && *pattern != ';' && *pattern != '*')
	{
		if (!eq(*pattern, *text_) && (*pattern != '?'))
			return false;
		pattern = inc(pattern);
		text_ = inc(text_);
	}
	if (*pattern != ';')
	{
		while (*text_)
		{
			if (*pattern == '*')
			{
				pattern = inc(pattern);
				if (!*pattern || *pattern == ';')
					return true;
				mp = pattern;
				cp = inc(text_);
			}
			else if (eq(*pattern, *text_) || (*pattern == '?'))
			{
				pattern = inc(pattern);
				text_ = inc(text_);
			}
			else
			{
				pattern = mp;
				text_ = cp;
				cp = inc(cp);
			}
		}
		while (*pattern == '*')
		{
			pattern = inc(pattern);
		}
	}
	if (*pattern == ';')
	{
		return true;
	}
	if (*pattern)
	{
		mp = pattern;
		while ((*mp) && (*mp != ';'))
		{
			mp = inc(mp);
		}
		if (*mp == ';')
		{
			pattern = inc(mp);
			return MatchPatternI(pattern, text);
		}
	}

	return !*pattern;
}

// Japanese Kanji filter module
// 	Portions Copyright (c) 2002, Atsuo Ishimoto.
// 	Portions Copyright (c) 1995 - 2000 Haruhiko Okumura.
// This function is based on pykf.

#define iseuc(c)		((c) >= 0xa1 && (c) <= 0xfe)
#define isgaiji1(c)		((c) >= 0xf0 && (c) <= 0xf9)
#define isibmgaiji1(c)	((c) >= 0xfa && (c) <= 0xfc)
#define issjis1(c)		(((c) >= 0x81 && (c) <= 0x9f) || ((c) >= 0xe0 && (c) <= 0xef) || isgaiji1(c) || isibmgaiji1(c))
#define issjis2(c)		((c) >= 0x40 && (c) <= 0xfc && (c)!=0x7f)
#define ishankana(c)	((c) >= 0xa0 && (c) <= 0xdf)

#define isutf8_2byte(c)	(0xc0 <= (c) && (c) <= 0xdf)
#define isutf8_3byte(c)	(0xe0 <= (c) && (c) <= 0xef)
#define isutf8_trail(c)	(0x80 <= (c) && (c) <= 0xbf)

Encoding GetEncoding(PCSTR str, size_t len)
{
	size_t	i;
	int	ascii, eucjp, sjis, utf8, bad_eucjp, bad_sjis, bad_utf8;
	int	jis, hankana;
	const unsigned char* buf = (const unsigned char*)str;

	ascii = 1;
	bad_eucjp = eucjp = 0; 
	bad_sjis = sjis = 0;
	bad_utf8 = utf8 = 0;
	jis = 0;

	// check BOM
	if (len >= 2)
	{
		if (buf[0] == 0xff && buf[1] == 0xfe)
			return ENCODING_UTF16_LE;
		else if (buf[0] == 0xfe && buf[1] == 0xff)
			return ENCODING_UTF16_BE;
	}
	if (len >= 3 && !memcmp(buf, "\xef\xbb\xbf", 3))
		return ENCODING_UTF8_BOM;

	// check ENCODING_SJIS
	hankana = 0;
	for (i = 0; i < len; i++)
	{
		if (buf[i] >= 0x80)
			ascii = 0;
		if (buf[i] == 0x1b)
			jis = 1;
		if (buf[i] == 0x8e &&
			i + 2 < len &&
			buf[i + 2] == 0x8e &&
			ishankana(buf[i + 1]))
		{
			bad_sjis += 1;
		}

		if (ishankana(buf[i]))
		{
			sjis += 0x10/2 - 1;
			hankana++;
		}
		else
		{
			if (hankana == 1)
				bad_sjis++;
			hankana = 0;

			if (issjis1(buf[i]))
			{
				if (i + 1 < len)
				{
					if (issjis2(buf[i + 1]))
					{
						sjis += 0x10;
						i++;
					}
					else
						bad_sjis += 0x100;
				}
			}
			else if (buf[i] >= 0x80)
				bad_sjis += 0x100;			
		}
	}

	if (ascii)
		return jis ? ENCODING_JIS : ENCODING_ASCII;

	// check ENCODING_EUCJP - JP
	hankana = 0;
	for (i = 0; i < len; i++)
	{
		if (buf[i] == 0x8e)
		{
			if (i + 1 < len)
			{
				if (ishankana(buf[i + 1]))
				{
					eucjp += 10; 
					i++;
					hankana++;
				}
				else
					bad_eucjp += 0x100;
			}
		}
		else
		{
			if (hankana == 1)
				bad_eucjp++;
			hankana = 0;
			if (iseuc(buf[i]))
			{
				if (i + 1 < len)
				{
					if (iseuc(buf[i + 1]))
					{
						i++;
						eucjp += 0x10;
					}
					else
						bad_eucjp += 0x100;
				}
			}
			else if (buf[i] == 0x8f)
			{
				if (i + 2 < len)
				{
					if (iseuc(buf[i + 1]) && iseuc(buf[i + 2]))
					{
						i += 2;
						eucjp += 0x10;
					}
					else
						bad_eucjp += 100;
				}
			}
			else if (buf[i] >= 0x80)
				bad_eucjp += 0x100;
		}
	}

	// check UTF-8
	for (i = 0; i < len; i++)
	{
		if (isutf8_2byte(buf[i]))
		{
			if (i + 1 < len)
			{
				if (isutf8_trail(buf[i + 1]))
				{
					utf8 += 10;
					i++;
				}
				else
					bad_utf8 += 100;
			}
		}
		else if (isutf8_3byte(buf[i]))
		{
			if (i + 2 < len)
			{
				if (isutf8_trail(buf[i + 1]) && isutf8_trail(buf[i + 2]))
				{
					utf8 += 15;
					i += 2;
				}
				else
					bad_utf8 += 1000;
			}
		}
		else if (buf[i] >= 0x80)
			bad_utf8 += 1000;
	}


	if (sjis - bad_sjis > eucjp - bad_eucjp)
	{
		if (sjis - bad_sjis > utf8 - bad_utf8)
			return ENCODING_SJIS;
		else if (sjis - bad_sjis < utf8 - bad_utf8)
			return ENCODING_UTF8;
	}

	if (sjis - bad_sjis < eucjp - bad_eucjp)
	{
		if (eucjp - bad_eucjp > utf8 - bad_utf8)
			return ENCODING_EUCJP;
		else if (eucjp - bad_eucjp < utf8 - bad_utf8)
			return ENCODING_UTF8;
	}

	return ENCODING_UNKNOWN;
}

//=============================================================================

void StringBuffer::append(WCHAR c) throw()
{
	reserve(m_end + 1);
	m_buffer[m_end] = c;
	++m_end;
}

void StringBuffer::append(PCWSTR data, size_t ln) throw()
{
	if (ln == 0)
		return;
	size_t end = m_end;
	resize(m_end + ln);
	memcpy(m_buffer + end, data, ln * sizeof(WCHAR));
}

void StringBuffer::append(PCWSTR data) throw()
{
	append(data, wcslen(data));
}

void StringBuffer::format(PCWSTR fmt, ...) throw()
{
	WCHAR buffer[1024];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(buffer, fmt, args);
	va_end(args);
	append(buffer);
}

void StringBuffer::resize(size_t sz) throw()
{
	reserve(sz);
	m_end = sz;
}

void StringBuffer::reserve(size_t capacity) throw()
{
	if (m_max >= capacity)
		return;
	m_max *= 2;
	if (m_max < capacity)
		m_max = capacity;
	if (m_max < 16)
		m_max = 16;
	m_buffer = (PWSTR)realloc(m_buffer, m_max * sizeof(WCHAR));
}

//=============================================================================

RingBuffer::RingBuffer()
{
	m_buffer = NULL;
	m_begin = m_end = m_max = 0;
}

RingBuffer::~RingBuffer()
{
	free(m_buffer);
}

char* RingBuffer::resize(size_t len)
{
	if (m_begin + len > m_max)
	{
		if (m_begin > len / 2)
		{
			size_t	size = m_end - m_begin;
			memmove(m_buffer, m_buffer + m_begin, size);
			m_begin = 0;
			m_end = size;
		}
		if (m_begin + len > m_max)
		{
			m_max = max(m_begin + len, m_max * 3 / 2);
			m_buffer = (char*)realloc(m_buffer, m_max);
		}
	}

	m_end = m_begin + len;
	return m_buffer + m_begin;
}

void RingBuffer::shift(size_t len)
{
	ASSERT(m_begin + len <= m_end);
	m_begin += len;
}
