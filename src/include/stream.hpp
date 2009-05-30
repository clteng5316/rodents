// stream.hpp
#pragma once

//==========================================================================================================================
// Stream

HRESULT	StreamCreate(IStream** pp, HGLOBAL* mem) throw();
HRESULT	StreamCreate(IStream** pp, IShellItem* item, DWORD stgm) throw();
HRESULT	StreamCreate(IStream** pp, PCWSTR filename, DWORD stgm) throw();

void	StreamRead(IStream* stream, void* data, size_t size) throw(HRESULT);
void	StreamWrite(IStream* stream, const void* data, size_t size) throw(HRESULT);
UINT64	StreamSeek(IStream* stream, INT64 off, DWORD whence = SEEK_SET);

template < typename T >
inline void StreamReadT(IStream* stream, T data[], size_t ln) throw(HRESULT)
{
	StreamRead(stream, data, ln * sizeof(T));
}

template < typename T >
inline void StreamWriteT(IStream* stream, const T data[], size_t ln) throw(HRESULT)
{
	StreamWrite(stream, data, ln * sizeof(T));
}
