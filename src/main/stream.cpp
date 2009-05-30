// stream.cpp

#include "stdafx.h"
#include "autoptr.hpp"
#include "stream.hpp"
#include "unknown.hpp"
#include "win32.hpp"

static inline bool STGM_IS_READ(DWORD stgm) throw()
{
	switch (stgm & (STGM_READ | STGM_WRITE | STGM_READWRITE))
	{
	case STGM_READ:
	case STGM_READWRITE:
		return true;
	default:
		return false;
	}
}

static inline bool STGM_IS_WRITE(DWORD stgm) throw()
{
	switch (stgm & (STGM_READ | STGM_WRITE | STGM_READWRITE))
	{
	case STGM_WRITE:
	case STGM_READWRITE:
		return true;
	default:
		return false;
	}
}

HRESULT	StreamCreate(IStream** pp, HGLOBAL* mem) throw()
{
	ASSERT(pp);
	ASSERT(mem);

	*pp = null;
	*mem = null;

	HRESULT hr;
	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, 0);
	if (!hGlobal)
		return HRESULT_FROM_WIN32(GetLastError());
	if SUCCEEDED(hr = ::CreateStreamOnHGlobal(hGlobal, false, pp))
	{
		*mem = hGlobal;
		return hr;
	}
	::GlobalFree(hGlobal);
	return hr;
}

HRESULT	StreamCreate(IStream** pp, PCWSTR filename, DWORD stgm) throw()
{
	WCHAR	path[MAX_PATH];
	LinkResolve(filename, path);
	return ::SHCreateStreamOnFileEx(path, stgm, FILE_ATTRIBUTE_NORMAL, 0, null, pp);
}

HRESULT	StreamCreate(IStream** pp, IShellItem* item, DWORD stgm) throw()
{
	ASSERT(pp);
	ASSERT(item);

	*pp = null;

	HRESULT			hr;

	// IShellItem 組み込みの IStream ハンドラを試す。
	// Vistaでのモバイルデバイス(PDA)上のアイテムなどに対して期待できる。
	ref<IBindCtx>	bind;
	if SUCCEEDED(CreateBindCtx(0, &bind))
	{
		BIND_OPTS tools = { sizeof(BIND_OPTS), 0, stgm, 0 };
		bind->SetBindOptions(&tools);
	}
	if SUCCEEDED(hr = item->BindToHandler(bind, BHID_Stream, IID_PPV_ARGS(pp)))
		return hr;

	// IStream ハンドラが実装されていなければ、ファイルパスから作成する。
	CoStr name;
	if SUCCEEDED(hr = item->GetDisplayName(SIGDN_FILESYSPATH, &name))
		return StreamCreate(pp, name, stgm);

	return hr;
}

void StreamRead(IStream* stream, void* data, size_t size) throw(HRESULT)
{
	ULONG done = 0;
	HRESULT hr = stream->Read(data, (ULONG)size, &done);
	if FAILED(hr)
		throw hr;
	if (done != (ULONG)size)
		throw STG_E_READFAULT;
}

void StreamWrite(IStream* stream, const void* data, size_t size) throw(HRESULT)
{
	ULONG done = 0;
	HRESULT hr = stream->Write(data, (ULONG)size, &done);
	if FAILED(hr)
		throw hr;
	if (done != (ULONG)size)
		throw STG_E_MEDIUMFULL;
}

UINT64 StreamSeek(IStream* stream, INT64 off, DWORD whence) throw(HRESULT)
{
	ULARGE_INTEGER done;
	HRESULT hr = stream->Seek((LARGE_INTEGER&)off, whence, &done);
	if FAILED(hr)
		throw hr;
	return done.QuadPart;
}
