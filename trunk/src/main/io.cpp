// io.cpp
#include "stdafx.h"
#include "ref.hpp"
#include "sq.hpp"
#include "stream.hpp"
#include "string.hpp"
#include "resource.h"

static const UINT CODEPAGE[] =
{
	CP_ACP,		// ENCODING_UNKNOWN
	CP_ACP,		// ENCODING_ASCII
	CP_SJIS,	// ENCODING_SJIS
	CP_EUCJP,	// ENCODING_EUCJP
	CP_JIS,		// ENCODING_JIS
	CP_UTF7,	// ENCODING_UTF7
	CP_UTF8,	// ENCODING_UTF8
	CP_UTF8,	// ENCODING_UTF8_BOM
};

//=============================================================================

class Reader : public object
{
private:
	ref<IStream>	m_stream;
	RingBuffer		m_buffer;
	Encoding		m_encoding;

public:
	void		constructor(sq::VM v);
	SQInteger	read(sq::VM v);
	SQInteger	_nexti(sq::VM v);

	void		set_stream(IStream* stream);
	bool		nextline(size_t* len, size_t* linelen);
	size_t		getline(WCHAR buffer[], size_t len, size_t linelen);

private:
	void	fill(size_t len);
};

//=============================================================================

class Writer : public object
{
private:
	ref<IStream>	m_stream;
	Encoding		m_encoding;

public:
	void		constructor(sq::VM v);
	void		print(PCWSTR value);
	void		write(PCWSTR value);

private:
	void		write_text(PCWSTR value, bool linebreak);
};

//=============================================================================

SQInteger def_io(sq::VM v)
{
#define CLASS	Reader
	v.newclass<Reader>();
	def_member(line);
	def_method(read);
	def_method(_nexti);
	v.newslot(-3);
#undef CLASS

#define CLASS	Writer
	v.newclass<Writer>();
	def_method(print);
	def_method(write);
	v.newslot(-3);
#undef CLASS

	return 1;
}

//=============================================================================

static void StreamCreate(sq::VM v, SQInteger idx, DWORD stgm, IStream** pp)
{
	HRESULT	hr;
	stgm |= STGM_SHARE_DENY_NONE;

	// as IUnknown
	if (sq_gettype(v, 2) == OT_USERDATA)
	{
		IUnknown* unk = v.get<IUnknown*>(2);;

		ref<IShellItem> item;
		if SUCCEEDED(unk->QueryInterface(&item))
			hr = StreamCreate(pp, item, stgm);
		else if SUCCEEDED(unk->QueryInterface(pp))
			hr = StreamCreate(pp, item, stgm);
		else
			hr = E_NOINTERFACE;
	}
	else
	{
		// as string
		hr = StreamCreate(pp, v.tostring(2), stgm);
	}

	if FAILED(hr)
		throw sq::Error(L"Cannot open as a stream");
}

const ULONG		HEADSIZE = 4096;
const ULONG		LINESIZE = 1024;
const size_t	END_OF_STREAM = (size_t)-1; 

void Reader::constructor(sq::VM v)
{
	ref<IStream> stream;
	StreamCreate(v, 2, STGM_READ, &stream);
	set_stream(stream);
}

void Reader::set_stream(IStream* stream)
{
	m_stream = stream;
	m_buffer.clear();

	// 最初のいくらかを読み込んで、エンコーディングを自動判定する。
	fill(HEADSIZE);
	m_encoding = GetEncoding(m_buffer, m_buffer.size());

	// BOM を捨てる。
	switch (m_encoding)
	{
	case ENCODING_UTF16_LE:
	case ENCODING_UTF16_BE:
//		m_buffer.shift(2);
		throw sq::Error(L"UTF16 is not supported");
		break;
	case ENCODING_UTF8_BOM:
		m_buffer.shift(3);
		break;
	}
}

SQInteger Reader::read(sq::VM v)
{
	PWSTR	dst;
	size_t	len, linelen;
	if (nextline(&len, &linelen))
	{
		dst = sq_getscratchpad(v, len * sizeof(WCHAR));
		len = getline(dst, len, linelen);
	}
	else
	{
		dst = null;
	}

	v.push(L"line");
	sq::push(v, dst, len);
	sq::rawset(v, 1);
	return dst ? 1 : 0;
}

SQInteger Reader::_nexti(sq::VM v)
{
	if (read(v) <= 0)
		return 0;
	v.push(L"line");
	return 1;
}

namespace str
{
	inline static size_t find(const char* buffer, char c, size_t begin, size_t end)
	{
		for (size_t i = begin; i < end; ++i)
		{
			if (buffer[i] == c)
				return i;
		}
		return END_OF_STREAM;
	}
}

bool Reader::nextline(size_t* len_, size_t* linelen_)
{
	size_t	linelen;
	size_t	tail;
	size_t	offset = 0;

	for (;;)
	{
		if ((tail = str::find(m_buffer, '\n', offset, m_buffer.size())) != END_OF_STREAM)
		{
			linelen = tail + 1;
			break;
		}
		else if (!m_stream)
		{
			linelen = tail = m_buffer.size();
			if (linelen == 0)
			{
				m_buffer.clear();
				*len_ = *linelen_ = 0;
				return false;
			}
			break;
		}
		fill(LINESIZE);
	}

	while (tail > 0 && m_buffer[tail - 1] == '\r')
		tail--;

	*len_ = tail;
	*linelen_ = linelen;
	return true;
}

size_t Reader::getline(WCHAR buffer[], size_t len, size_t linelen)
{
	size_t ret = MultiByteToWideChar(CODEPAGE[m_encoding], 0, m_buffer, (int)len, buffer, (int)len);
	m_buffer.shift(linelen);
	return ret;
}

void Reader::fill(size_t len)
{
	ASSERT(m_stream);

	ULONG	done = 0;
	size_t	size = m_buffer.size();
	m_buffer.resize(size + len);
	HRESULT hr = m_stream->Read(&m_buffer[size], (ULONG)len, &done);
	m_buffer.resize(size + done);
	if (FAILED(hr) || done == 0)
		m_stream = null; // eof
}

//=============================================================================

static const struct
{
	PCWSTR		name;
	Encoding	encoding;
}
ENCODING_NAMES[] =
{
	{ L"ascii"		, ENCODING_ASCII },
	{ L"sjis"		, ENCODING_SJIS },
	{ L"shiftjis"	, ENCODING_SJIS },
	{ L"euc"		, ENCODING_EUCJP },
	{ L"eucjp"		, ENCODING_EUCJP },
	{ L"jis"		, ENCODING_JIS },
	{ L"utf8n"		, ENCODING_UTF8 },
	{ L"utf8"		, ENCODING_UTF8_BOM },
};

static PCWSTR fuzzy_skip(PCWSTR s)
{
	while (*s && wcschr(L" _-", *s))
		s++;
	return s;
}

// スペース, _, - と大文字小文字を無視して比較する
static bool fuzzy_eq(PCWSTR lhs, PCWSTR rhs)
{
	for (; *lhs || *rhs; lhs++, rhs++)
	{
		lhs = fuzzy_skip(lhs);
		rhs = fuzzy_skip(rhs);
		if (towlower(*lhs) != towlower(*rhs))
			return false;
	}
	lhs = fuzzy_skip(lhs);
	rhs = fuzzy_skip(rhs);
	return !*lhs && !*rhs;
}

static Encoding parse_encoding_name(PCWSTR name) throw(...)
{
	for (size_t i = 0; i < lengthof(ENCODING_NAMES); ++i)
	{
		if (fuzzy_eq(ENCODING_NAMES[i].name, name))
			return ENCODING_NAMES[i].encoding;
	}
	throw sq::Error(L"invalid encoding name");
}

void Writer::constructor(sq::VM v)
{
	if (v.gettop() < 3)
		m_encoding = ENCODING_UTF8_BOM;
	else
		m_encoding = parse_encoding_name(v.get<PCWSTR>(3));
	StreamCreate(v, 2, STGM_WRITE | STGM_CREATE, &m_stream);

	// BOM を書く。
	switch (m_encoding)
	{
	case ENCODING_UTF16_LE:
//		m_stream->Write("\xff\xfe", 2, NULL);
		throw sq::Error(L"UTF16 is not supported");
		break;
	case ENCODING_UTF16_BE:
//		m_stream->Write("\xfe\xff", 2, NULL);
		throw sq::Error(L"UTF16 is not supported");
		break;
	case ENCODING_UTF8_BOM:
		m_stream->Write("\xef\xbb\xbf", 3, NULL);
		break;
	}
}

void Writer::print(PCWSTR value)
{
	write_text(value, false);
}

void Writer::write(PCWSTR value)
{
	write_text(value, true);
}

void Writer::write_text(PCWSTR value, bool linebreak)
{
	if (!m_stream)
		throw sq::Error(L"already closed");

	int	srclen = (value ? (int)wcslen(value) : 0);
	int	dstlen = srclen * 4 + 2;

	PSTR	dst = (PSTR)sq_getscratchpad(sq::vm, dstlen);
	ULONG	len = WideCharToMultiByte(CODEPAGE[m_encoding], 0, value, srclen, dst, dstlen, NULL, NULL);

	// Convert LF to CR+LF.
	for (ULONG i = 0; i < len; ++i)
	{
		if (dst[i] == '\n' && (i == 0 || dst[i - 1] != '\r'))
		{
			// shift to right by 1 char.
			memmove(&dst[i + 1], &dst[i], len - i);
			dst[i] = '\r';
			++len;
			++i;
		}
	}

	// Add linkbreak if needed.
	if (linebreak)
	{
		dst[len    ] = '\r';
		dst[len + 1] = '\n';
		len += 2;
	}

	ULONG	done = 0;
	if (FAILED(m_stream->Write(dst, len, &done)) || len != done)
		throw sq::Error(L"write failed");
}

//=============================================================================

#define E_FILENOTFOULD		_HRESULT_TYPEDEF_(0x80070002L)

static HRESULT load(HSQUIRRELVM v, PCWSTR filename) throw()
{
	struct CharReader
	{
		Reader			reader;
		StringBuffer	line;
		size_t			offset;

		static SQInteger read(SQUserPointer file)
		{
			CharReader* r = (CharReader*)file;
			if (r->offset >= r->line.size())
			{
				size_t len, linelen;
				if (!r->reader.nextline(&len, &linelen))
					return 0;
				r->line.resize(len + 1);
				len = r->reader.getline(r->line.data(), len, linelen);
				r->line[len] = '\n';
				r->line.resize(len + 1);
				r->offset = 0;
			}
			return r->line[r->offset++];
		}
	};

	WCHAR	path[MAX_PATH];
	if (PathIsRelative(filename))
		PathCombine(path, PATH_SCRIPT, filename);
	else
		wcscpy_s(path, filename);
	PathAddExtension(path, L".nut");

	ref<IStream> stream;
	HRESULT hr = StreamCreate(&stream, path, STGM_READ | STGM_SHARE_DENY_NONE);
	if SUCCEEDED(hr)
	{
		CharReader r;
		r.offset = 0;
		r.reader.set_stream(stream);
		if SQ_SUCCEEDED(sq_compile(v, &CharReader::read, &r, path, SQTrue))
			return S_OK;
		else
			return E_FAIL;
	}
	else if (hr == E_FILENOTFOULD || hr == STG_E_FILENOTFOUND)
	{
		// do not throw if file not found.
		return S_FALSE;
	}
	else
	{
		sq::raise(v, _S(STR_E_NOFILE), path);
		return hr;
	}
}

SQInteger sq::import(HSQUIRRELVM v, PCWSTR filename) throw()
{
	SQInteger top = sq_gettop(v);

	switch (load(v, filename))
	{
	case S_OK:
		sq_push(v, 1); // repush this
		sq::call(v, 1, SQFalse);
		sq_settop(v, top);
		sq::push(v, true);
		return 1;
	case S_FALSE:
		sq_settop(v, top);
		sq::push(v, false);
		return 1;
	default:
		return SQ_ERROR;
	}
}
