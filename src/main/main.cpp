// main.cpp
#include "stdafx.h"
#include "sq.hpp"
#include "win32.hpp"
#include "ref.hpp"
#include "string.hpp"
#include "resource.h"

#pragma comment(lib, "comctl32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "imm32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "version")
#pragma comment(lib, "winmm")
#pragma comment(lib, "gdiplus")

// Ç±ÇÍÇ¡ÇƒÉoÉOÇ∂Ç·Ç»Ç¢ÇÃÅH
#define CLSID_NameSpaceTreeControl	CLSID_NamespaceTreeControl

const bool DEBUG = RELEASE_OR_DEBUG(false, true);
int		WINDOWS_VERSION;
int		VERSION;

#define def_(name)	v.def(_SC(#name), name)

const PCWSTR DIR_SCRIPT		= L"lib";
const PCWSTR DIR_DOCUMENT	= L"doc";

WCHAR APPNAME[MAX_PATH];
WCHAR PATH_ROOT[MAX_PATH];
WCHAR PATH_SCRIPT[MAX_PATH];

static SQInteger import(HSQUIRRELVM v) throw()
{
	PCWSTR name;
	if SQ_FAILED(sq_getstring(v, 2, &name))
		return SQ_ERROR;
	return sq::import(v, name);
}

static void init_constants()
{
	::GetModuleFileName(null, PATH_ROOT, MAX_PATH);

	// VERSION
	Version ver;
	if (ver.open(PATH_ROOT))
	{
		VERSION = ver.ProductVersion;
		wcscpy_s(APPNAME, ver.value(L"ProductName"));
	}

	// WINDOWS_VERSION
	OSVERSIONINFOEX info = { sizeof(OSVERSIONINFOEX) };
	::GetVersionEx((OSVERSIONINFO*)&info);

	DWORD release;
	if (info.wProductType == VER_NT_WORKSTATION)
		release = 0;
	else
		release = (GetSystemMetrics(SM_SERVERR2) == 0 ? 1 : 2);

	// {major}{minor}{NT}{SP}
	WINDOWS_VERSION = ((info.dwMajorVersion    << 24) +
					   (info.dwMinorVersion    << 16) +
					   (release <<  8) +
					   (info.wServicePackMajor));

	// Paths
	::PathRemoveFileSpec(PATH_ROOT);
	::SetCurrentDirectory(PATH_ROOT);
	::PathCombine(PATH_SCRIPT, PATH_ROOT, DIR_SCRIPT);
}

static int month2int(PCSTR name)
{
	const PCSTR MONTHS[12] =
	{
		"jan", "feb", "mar", "apr", "may", "jun",
		"jul", "aug", "sep", "oct", "nov", "dec"
	};

	for (int i = 0; i < lengthof(MONTHS); ++i)
		if (_strnicmp(name, MONTHS[i], 3) == 0)
			return i + 1;
	return 0;
}

static string iso_date(PCSTR date)
{
	int		year, month, day;
	CHAR	monstr[32];
	WCHAR	isostr[32];

	sscanf_s(date, "%32s %d %d", monstr, lengthof(monstr), &day, &year);
	month = month2int(monstr);
	swprintf_s(isostr, L"%04d-%02d-%02d", year, month, day);
	return str::dup(isostr);
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR args, int)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	init_constants();

	CoInitializeEx(null, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	OleInitialize(null);

	static ULONG_PTR gdiplusToken = 0;
	GdiplusStartupInput gdiplusInput;
	GdiplusStartup(&gdiplusToken, &gdiplusInput, null);

	sq::VM v = sq::open();

	sq_pushroottable(v);

	v.def(L"getslot"	, sq::getslot);
	v.def(L"import"		, import);

	sq_pushconsttable(v);
	def_(APPNAME);
	def_(DEBUG);
	def_(VERSION);
	def_(SQUIRREL_VERSION);
	def_(WINDOWS_VERSION);
	v.def(L"BUILD", iso_date(__DATE__));
	v.pop(1);

	def_config(v);
	def_io(v);
	def_os(v);
	def_ui(v);

	sq::import(v, L"main");

	BoardcastMessage(WM_DESTROY);
	v.close();
	DllUnload();

	GdiplusShutdown(gdiplusToken);
	OleUninitialize();
	CoUninitialize();

	return 0;
}

//#define ALWAYS_XP

namespace
{
	struct REGCLASS
	{
		CLSID	clsid;
		HRESULT (*create)(REFINTF pp);
	};

#define DEF_CLASS(name)		{ CLSID_##name, Xp##name##Create }

	const REGCLASS CLASS[] =
	{
		DEF_CLASS( ExplorerBrowser ),
		DEF_CLASS( KnownFolderManager ),
		DEF_CLASS( NameSpaceTreeControl ),
		DEF_CLASS( FileOperation ),
		DEF_CLASS( FileOpenDialog ),
		DEF_CLASS( FileSaveDialog ),
	};


	static HRESULT XpCreateInstance(REFCLSID clsid, REFINTF pp)
	{
		for (size_t i = 0; i < lengthof(CLASS); ++i)
			if (CLASS[i].clsid == clsid)
				return CLASS[i].create(pp);
		return REGDB_E_CLASSNOTREG;
	}
}

//#define ALWAYS_XP

HRESULT	CreateInstance(REFCLSID clsid, REFINTF pp)
{
	HRESULT hr;
#ifdef ALWAYS_XP
	if SUCCEEDED(XpCreateInstance(clsid, pp))
		return S_OK;
#endif
	hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC, pp.iid, pp.pp);
#ifndef ALWAYS_XP
	if (hr == REGDB_E_CLASSNOTREG)
		hr = XpCreateInstance(clsid, pp);
#endif
	return hr;
}
