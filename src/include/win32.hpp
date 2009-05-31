// win32.hpp
#pragma once

#include "autoptr.hpp"

//==========================================================================================================================

template < typename T, BOOL (STDMETHODCALLTYPE *Free)(T)  >
class Handle
{
protected:
	T	m_handle;

public:
	Handle(T handle = null) : m_handle(handle)	{}
	~Handle()	{ dispose(); }

	Handle& operator = (T handle) throw()	{ attach(handle); return *this; }
	operator T () const throw()		{ return m_handle; }

	void dispose() throw()
	{
		if (m_handle)
		{
			Free(m_handle);
			m_handle = null;
		}
	}

	void attach(T handle) throw()
	{
		if (m_handle != handle)
		{
			dispose();
			m_handle = handle;
		}
	}

	T detach() throw()
	{
		T tmp = m_handle;
		m_handle = null;
		return tmp;
	}

private:
	Handle(const Handle&);
	Handle& operator = (const Handle&);
};

extern bool DynamicLoadVoidPtr(void** fn, PCWSTR module, PCSTR name, int index = 0) throw();

template < typename T >
inline bool DynamicLoad(T** fn, PCWSTR module, PCSTR name, int index = 0) throw()
{
	return DynamicLoadVoidPtr((void**)fn, module, name, index);
}

//==========================================================================================================================
// System

extern int	WINDOWS_VERSION;
const int	WINDOWS_2000	= 0x05000000;
const int	WINDOWS_XP		= 0x05010000;
const int	WINDOWS_2003	= 0x05020000;
const int	WINDOWS_VISTA	= 0x06000000;

//==========================================================================================================================
// Key

#define VK_BUTTON_L			VK_LBUTTON	// 0x01 
#define VK_BUTTON_R			VK_RBUTTON	// 0x02
#define VK_BUTTON_M			VK_MBUTTON	// 0x04
#define VK_BUTTON_X			VK_XBUTTON1	// 0x05
#define VK_BUTTON_Y			VK_XBUTTON2	// 0x06
#define VK_DBLCLK(n)		(n + 0x96)
#define VK_BUTTON_LL		VK_DBLCLK(VK_BUTTON_L)
#define VK_BUTTON_RR		VK_DBLCLK(VK_BUTTON_R)
#define VK_BUTTON_MM		VK_DBLCLK(VK_BUTTON_M)
#define VK_BUTTON_XX		VK_DBLCLK(VK_BUTTON_X)
#define VK_BUTTON_YY		VK_DBLCLK(VK_BUTTON_Y)
#define VK_WHEEL_LEFT		0x88
#define VK_WHEEL_UP			0x89
#define VK_WHEEL_RIGHT		0x8A
#define VK_WHEEL_DOWN		0x8B

#define MK_ALTKEY		0x0080	// MK_ALT は使ってはならない！
#define MK_WINKEY		0x0100		
#define MK_KEYS			(MK_SHIFT | MK_CONTROL | MK_ALTKEY | MK_WINKEY)
#define MK_BUTTONS		(MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2)

#define GET_WHEEL(wParam, up, down)	((GET_WHEEL_DELTA_WPARAM(wParam) > 0) ? (up) : (down))
#define GET_XBUTTON(wParam, x1, x2)	((GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? (x1) : (x2))

extern INT		MSG2VK(UINT msg, WPARAM wParam) throw();
extern INT		MSG2VK(const MSG& msg) throw();
extern INT		MSG2MK(UINT msg, WPARAM wParam) throw();
extern INT		MSG2MK(const MSG& msg) throw();
inline bool		IsKeyPressed(int vkey) throw()	{ return 0 != (::GetKeyState(vkey) & 0x8000); }
extern void		SetKeyState(UINT vk, UINT mk);
extern void		ResetKeyState(UINT vk);
extern UINT		GetModifiers();
extern UINT		GetButtons();
inline POINT	GetCursorPos() throw()	{ POINT pt; ::GetCursorPos(&pt); return pt; }
extern bool		IsMenuWindow(HWND hwnd);

//==========================================================================================================================
// System

extern HINSTANCE	DllLoad(PCWSTR name) throw();
extern void			DllUnload() throw();
extern HWND			GetWindow() throw();
extern void			MouseDrag(UINT mk, POINT from, POINT to) throw();
extern void			MouseUp(UINT mk, POINT pt) throw();

//==========================================================================================================================
// Window

enum
{
	WM_NOTIFYFOCUS = WM_USER + 1000,
	WM_SHNOTIFY,
};

#undef CreateWindow

extern int		Alert(PCWSTR message, PCWSTR details, PCWSTR title, UINT type) throw();
extern void		ActivateWindow(HWND hwnd) throw();
extern void		BoardcastMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) throw();
extern void		CenterWindow(HWND hwnd, HWND outer = null) throw();
extern HWND		CreateWindow(const WNDCLASSEX& wc, const CREATESTRUCT& cs, SUBCLASSPROC wndproc) throw();
extern HWND		FindWindowByClass(HWND hwnd, PCWSTR name) throw();
extern SIZE		GetClientSize(HWND hwnd) throw();
extern DWORD	GetStyle(HWND hwnd) throw();
extern DWORD	GetStyleEx(HWND hwnd) throw();
extern bool		GetVisible(HWND hwnd) throw();
extern bool		HasFocus(HWND hwnd) throw();
extern void		InitCommonControls(UINT flags) throw();
extern void		PumpMessage() throw();
extern void		ScreenToClient(HWND hwnd, RECT* rc) throw();
extern void		SetClientSize(HWND hwnd, int w, int h) throw();
extern void		SetRedraw(HWND hwnd, bool redraw) throw();
extern WNDPROC	SetWindowProc(HWND hwnd, WNDPROC wndproc);
extern UINT		SendMessageToChildren(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) throw();
extern void		Sound(PCWSTR path) throw();
extern UINT		PostMessageToChildren(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) throw();

class LockRedraw
{
private:
	HWND	m_hwnd;
public:
	LockRedraw(HWND hwnd);
	~LockRedraw();
};

//==========================================================================================================================
// GDI

extern BOOL __stdcall DeleteBitmap(HBITMAP handle) throw();
extern BOOL __stdcall DeleteBrush(HBRUSH handle) throw();
extern BOOL __stdcall DeleteFont(HFONT handle) throw();
extern BOOL __stdcall DeletePen(HPEN handle) throw();

typedef Handle<HDC    , DeleteDC>		DC;
typedef Handle<HBITMAP, DeleteBitmap>	BitmapH;
typedef Handle<HBRUSH , DeleteBrush>	BrushH;
typedef Handle<HFONT  , DeleteFont>		FontH;
typedef Handle<HPEN   , DeletePen>		PenH;

inline HBRUSH	GetStockBrush(int n)	{ return (HBRUSH)::GetStockObject(n); }
inline HFONT	GetStockFont(int n)		{ return (HFONT)::GetStockObject(n); }
inline HPEN		GetStockPen(int n)		{ return (HPEN)::GetStockObject(n); }

inline HBITMAP	SelectBitmap(HDC dc, HBITMAP obj)	{ ASSERT(dc); return (HBITMAP)::SelectObject(dc, obj); }
inline HBRUSH	SelectBrush(HDC dc, HBITMAP obj)	{ ASSERT(dc); return (HBRUSH)::SelectObject(dc, obj); }
inline HFONT	SelectFont(HDC dc, HFONT obj)		{ ASSERT(dc); return (HFONT)::SelectObject(dc, obj); }

extern void		GetNonClientMetrics(NONCLIENTMETRICS* metrics);
extern SIZE		GetTextExtent(HDC dc, HFONT font, PCWSTR text) throw();
extern SIZE		BitmapSize(HBITMAP bitmap) throw();

extern HBITMAP		BitmapLoad(IStream* stream, const SIZE* sz = null) throw();
extern HBITMAP		BitmapLoad(PCWSTR filename, const SIZE* sz = null) throw();
extern HBITMAP		BitmapLoad(Image* image, const SIZE* sz = null) throw();
extern HIMAGELIST	ImageListLoad(IStream* stream) throw();
extern HIMAGELIST	ImageListLoad(PCWSTR filename) throw();
extern HIMAGELIST	ImageListLoad(Bitmap* image) throw();

//==========================================================================================================================
// Menu

const HRESULT S_RENAME	= 2;	// IContextMenu.Rename が選択された。呼び出し側で対応すること。
const HRESULT S_DEFAULT	= 3;	// デフォルト項目が選択された。呼び出し側で対応すること。

typedef Handle<HMENU, DestroyMenu>		MenuH;
typedef void (*MenuFill)(HMENU menu, UINT id);

HMENU	MenuAppend(HMENU menu, UINT flags, UINT id, PCWSTR name);
int		MenuPopup(POINT ptScreen, MenuFill fill) throw();
HRESULT MenuPopup(POINT pt, IContextMenu* menu, PCWSTR defaultCommand) throw();
HRESULT MenuPopup(POINT pt, IShellView* view, ICommDlgBrowser2* browser) throw();

//==========================================================================================================================
// Special Folders

#define FOLDER_COMPUTER		L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"	// My Computer
#define FOLDER_PROFILE		L"::{ECF03A32-103D-11d2-854D-006008059367}"	// 
#define FOLDER_PERSONAL		L"::{450D8FBA-AD25-11D0-98A8-0800361B1103}"	// My Documents
#define FOLDER_NETWORK		L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}"
#define FOLDER_TRASH		L"::{645FF040-5081-101B-9F08-00AA002F954E}"
#define FOLDER_CONTROL		L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}"	// Control Panel

//==========================================================================================================================
// ITEMIDLIST

typedef AutoPtr<ITEMIDLIST, CoTaskMem> ILPtr;

// UINT ILGetSize(const ITEMIDLIST* pidl);
// BOOL ILIsEqual(const ITEMIDLIST* pidl1, const ITEMIDLIST* pidl2);
// BOOL ILIsParent(const ITEMIDLIST* pidlParent, const ITEMIDLIST* pidlBelow, BOOL fImmediate);
// ITEMIDLIST* ILGetNext(const ITEMIDLIST* pidl);
// ITEMIDLIST* ILFindChild(const ITEMIDLIST* pidlParent, const ITEMIDLIST* pidlChild);
// ITEMIDLIST* ILFindLastID(const ITEMIDLIST* pidl);

// BOOL ILRemoveLastID(ITEMIDLIST* pidl);
// ITEMIDLIST* ILAppendID(ITEMIDLIST* pidl, LPSHITEMID pmkid, BOOL fAppend_or_Prepend);
// ITEMIDLIST* ILClone(const ITEMIDLIST* pidl);
// ITEMIDLIST* ILCloneFirst(const ITEMIDLIST* pidl);
// ITEMIDLIST* ILCombine(const ITEMIDLIST* pidl1, const ITEMIDLIST* pidl2);

inline bool		ILIsRoot(const ITEMIDLIST* item) throw()	{ return item->mkid.cb == 0; }
inline void		ILDelete(const ITEMIDLIST* item) throw()	{ ::CoTaskMemFree((void*)item); }
ILPtr			ILCreate(PCWSTR path) throw();
ILPtr			ILCreate(IShellFolder* folder, PCWSTR relname) throw();
ILPtr			ILCreate(const CIDA* items, size_t index) throw();
ILPtr			ILCreate(IPersistIDList* presist) throw();
ILPtr			ILCreate(IUnknown* obj) throw();
ILPtr			ILCloneParent(const ITEMIDLIST* item) throw();
inline HRESULT	ILPath(const ITEMIDLIST* pidl, CHAR  path[MAX_PATH]) throw()	{ return SHGetPathFromIDListA(pidl, path) ? S_OK : E_FAIL; }
inline HRESULT	ILPath(const ITEMIDLIST* pidl, WCHAR path[MAX_PATH]) throw()	{ return SHGetPathFromIDListW(pidl, path) ? S_OK : E_FAIL; }
HRESULT			ILStat(const ITEMIDLIST* item, WIN32_FIND_DATA* stat) throw();
HRESULT			ILParentFolder(IShellFolder** pp, const ITEMIDLIST* item, const ITEMIDLIST** leaf = null) throw();
HRESULT			ILFolder(IShellFolder** pp, const ITEMIDLIST* item) throw();
HRESULT			ILContextMenu(IContextMenu** pp, const ITEMIDLIST* item) throw();
HRESULT			ILDisplayName(IShellFolder* folder, const ITEMIDLIST* leaf, DWORD shgdn, WCHAR name[MAX_PATH]);
DWORD_PTR		ILFileInfo(const ITEMIDLIST* pidl, SHFILEINFO* info, DWORD flags, DWORD dwFileAttr = 0);
int				ILIconIndex(const ITEMIDLIST* pidl);
HRESULT			ILExecute(const ITEMIDLIST* pidl, PCWSTR verb = null, PCWSTR args = null, PCWSTR dir = null);
int				ILCompare(const ITEMIDLIST* lhs, const ITEMIDLIST* rhs) throw();
bool			ILEquals(const ITEMIDLIST* lhs, const ITEMIDLIST* rhs) throw();
bool			ILEquals(const ITEMIDLIST* item, UINT csidl);
bool			ILIsDescendant(const ITEMIDLIST* item, UINT csidl);
const ITEMIDLIST*	ILGetCache(UINT csidl);

//==========================================================================================================================
// CIDA

typedef AutoPtr<CIDA, CoTaskMem>	CIDAPtr;

CIDAPtr				CIDACreate(IDataObject* data) throw();
CIDAPtr				CIDACreate(IShellView* view, int svgio) throw();
CIDAPtr				CIDACreate(IFolderView* view, int svgio) throw();
CIDAPtr				CIDACreate(const ITEMIDLIST* parent, size_t count, PCUITEMID_CHILD_ARRAY children);
CIDAPtr				CIDAClone(const CIDA* items) throw();
const ITEMIDLIST*	CIDAParent(const CIDA* items) throw();
const ITEMIDLIST*	CIDAChild(const CIDA* items, size_t index) throw();
inline size_t		CIDACount(const CIDA* items) throw()	{ return items ? items->cidl : 0; }
size_t				CIDASize(const CIDA* items) throw();
inline void			CIDAFree(const CIDA* items) throw()		{ CoTaskMemFree((void*)items); }

//==========================================================================================================================
// IShellItem

HRESULT PathCreate(IShellItem** pp, PCWSTR path);
HRESULT	PathCreate(IShellItem** pp, IShellFolder* folder, const ITEMIDLIST* parent, const ITEMIDLIST* leaf);
HRESULT	PathCreate(IShellItem** pp, const ITEMIDLIST* item);
HRESULT	PathCreate(IShellItem** pp, IShellItem* parent, PCWSTR name);
HRESULT	PathGetBrowsable(IShellItem* item) throw();
HBITMAP	PathGetThumbnail(IShellItem* item) throw();
HRESULT PathStat(IShellItem* item, WIN32_FIND_DATA* stat) throw();

//==========================================================================================================================
// IShellItemArray

HRESULT PathsCreate(IShellItemArray** pp, IDataObject* data);
HRESULT PathsCreate(IShellItemArray** pp, IShellView* view, int svgio);
HRESULT PathsCreate(IShellItemArray** pp, const ITEMIDLIST* parent, IShellFolder* folder, IEnumIDList* items);

//==========================================================================================================================
// IKnownFolder

HRESULT KnownFolderCreate(IKnownFolder** pp, PCWSTR path, PCWSTR* leaf);
void	KnownFolderResolve(WCHAR dst[MAX_PATH], PCWSTR src);
string	KnownFolderEncode(PCWSTR path);

//==========================================================================================================================
// Registry

HRESULT	RegGetString(HKEY hKey, PCWSTR subkey, PCWSTR value, WCHAR outString[], size_t cch);
template < size_t sz >
inline HRESULT RegGetString(HKEY hKey, PCWSTR subkey, PCWSTR value, WCHAR (&outString)[sz])
{
	return RegGetString(hKey, subkey, value, outString, sz);
}
HRESULT	RegGetAssocExe(PCWSTR extension, WCHAR exe[MAX_PATH]);

//==========================================================================================================================
// Link (Shortcut)

HRESULT	LinkResolve(PCWSTR src, ITEMIDLIST** dst, IShellLink** pp = null) throw();
HRESULT	LinkResolve(PCWSTR src, WCHAR dst[MAX_PATH], IShellLink** pp = null) throw();
HRESULT	LinkCreate(const ITEMIDLIST* item, PCWSTR dst, IShellLink** pp = null) throw();

//==========================================================================================================================
// Filesystem Observer

void	SHNotifyBegin(HWND hwnd, UINT msg);
void	SHNotifyEnd();
HRESULT	SHNotifyMessage(WPARAM wParam, LPARAM lParam);

//==========================================================================================================================
// File Operations

int		FileOperate(UINT what, FILEOP_FLAGS flags, const WCHAR src[], const WCHAR dst[]) throw();

HRESULT	FileOperate(IFileOperation** pp);

HRESULT FileRename(IFileOperation* op, IShellItem* item, PCWSTR name);
HRESULT FileRename(IFileOperation* op, IUnknown* items, PCWSTR name);
HRESULT FileRename(IFileOperation* op, PCWSTR item, PCWSTR name);
HRESULT FileRename(IFileOperation* op, size_t num, PCWSTR items[], PCWSTR name);

HRESULT FileMove(IFileOperation* op, IShellItem* item, IShellItem* folder, PCWSTR name);
HRESULT FileMove(IFileOperation* op, IUnknown* items, IShellItem* folder);
HRESULT FileMove(IFileOperation* op, PCWSTR item, PCWSTR path);
HRESULT FileMove(IFileOperation* op, size_t num, PCWSTR items[], PCWSTR folder);

HRESULT FileCopy(IFileOperation* op, IShellItem* item, IShellItem* folder, PCWSTR name);
HRESULT FileCopy(IFileOperation* op, IUnknown* items, IShellItem* folder);
HRESULT FileCopy(IFileOperation* op, PCWSTR item, PCWSTR path);
HRESULT FileCopy(IFileOperation* op, size_t num, PCWSTR items[], PCWSTR folder);

HRESULT FileDelete(IFileOperation* op, IShellItem* item);
HRESULT FileDelete(IFileOperation* op, IUnknown* items);
HRESULT FileDelete(IFileOperation* op, PCWSTR item);
HRESULT FileDelete(IFileOperation* op, size_t num, PCWSTR items[]);

HRESULT	FileNew(IFileOperation* op, IShellItem* folder, PCWSTR name) throw();
HRESULT	FileNew(IFileOperation* op, PCWSTR path) throw();
HRESULT	FileNew(IShellView* view, bool folder);

HRESULT	FileCut(PCWSTR src);
HRESULT	FileCopy(PCWSTR src);
HRESULT	FilePaste(PCWSTR dst);
HRESULT	FileDialogSetPath(PCWSTR path);

//==========================================================================================================================
// Clipboard

extern string	ClipboardGetText();
extern void		ClipboardSetText(PCWSTR text);

//==========================================================================================================================
// Version

class Version
{
private:
	BYTE*	m_info;
	WORD	m_language;
	WORD	m_codepage;
public:
	int		FileVersion;
	int		ProductVersion;
public:
	Version();
	~Version();
	bool	open(PCWSTR filename);
	void	close();
	PCWSTR	value(PCWSTR what) const throw();
};

//==========================================================================================================================

HIMAGELIST SHGetImageList(int size);

inline UINT64 FileSizeOf(const WIN32_FIND_DATA& info) throw()
{
	return (info.nFileSizeHigh * ((UINT64)MAXDWORD + 1)) + info.nFileSizeLow;
}

//==========================================================================================================================
// XP

extern HRESULT XpCreateShellItemArray(IShellItemArray** pp, const CIDA* cida, IShellFolder* folder);
extern HRESULT XpCreateShellItemArray(IShellItemArray** pp, const ITEMIDLIST* parent, IShellFolder* folder, size_t count, ITEMIDLIST** children);
extern HRESULT XpExplorerBrowserCreate(REFINTF pp);
extern HRESULT XpKnownFolderManagerCreate(REFINTF pp);
extern HRESULT XpNameSpaceTreeControlCreate(REFINTF pp);
extern HRESULT XpFileOpenDialogCreate(REFINTF pp);
extern HRESULT XpFileSaveDialogCreate(REFINTF pp);
extern HRESULT XpFileOperationCreate(REFINTF pp);

#define CSIDL_GAMES		0xF0
#define CSIDL_LINKS		0xF1

//==========================================================================================================================
// PROPERTYKEY

#define DECLARE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) const PROPERTYKEY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
// コントロールパネル: カテゴリ
DECLARE_PROPERTYKEY(PKEY_Controls_Category, 0x305CA226, 0xD286, 0x468E, 0xB8, 0x48, 0x2B, 0x2E, 0x8E, 0x69, 0x7B, 0x74, 2);
// ごみ箱: 元の場所
DECLARE_PROPERTYKEY(PKEY_Trash_Where, 0x9B174B33, 0x40FF, 0x11D2, 0xA2, 0x7E, 0x00, 0xC0, 0x4F, 0xC3, 0x08, 0x71, 2);
// ごみ箱: 削除日時
DECLARE_PROPERTYKEY(PKEY_Trash_When, 0x9B174B33, 0x40FF, 0x11D2, 0xA2, 0x7E, 0x00, 0xC0, 0x4F, 0xC3, 0x08, 0x71, 3);
// ゲーム: 最終プレイ日
DECLARE_PROPERTYKEY(PKEY_Game_LastPlay, 0x26B8D54F, 0x371F, 0x4AEB, 0x8A, 0x84, 0x92, 0x24, 0xAE, 0xA4, 0xD4, 0x0A, 36);
// ゲーム: インストールの場所
DECLARE_PROPERTYKEY(PKEY_Game_Where, 0x841E4F90, 0xFF59, 0x4D16, 0x89, 0x47, 0xE8, 0x1B, 0xBF, 0xFA, 0xB3, 0x6D, 9);
