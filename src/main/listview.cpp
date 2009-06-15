// listview.cpp
#include "stdafx.h"
#include "listview.hpp"
#include "sq.hpp"
#include "string.hpp"
#include "resource.h"

//==========================================================================================================================

const UINT FWF_SELECT[] =
{
	0,
	FWF_FULLROWSELECT,
	FWF_FULLROWSELECT | FWF_CHECKSELECT,
	FWF_FULLROWSELECT | FWF_TRICHECKSELECT,
};

enum FP
{
	FP_DISABLE,
	FP_SELECTION,
	FP_HOTTRACK,
};

#define FWF_MASK			(FWF_FULLROWSELECT | FWF_CHECKSELECT | FWF_TRICHECKSELECT)
#define FWF_DEFAULT			(FWF_SHOWSELALWAYS | FWF_NOWEBVIEW | FWF_AUTOARRANGE)
#define RETRY_ADJUST		5		// 5回
#define TIMER_ADJUST		1000
#define TIMER_ADJUST_MAX	(TIMER_ADJUST + RETRY_ADJUST)
#define TIMER_PREVIEW		1100
#define DELAY_ADJUST		100		// 100ms
#define DELAY_PREVIEW		50		// カスタマイズできたほうが良い? 
#define PREVIEW_ALPHA		255		// カスタマイズできたほうが良い?

//==========================================================================================================================
// global configuration parameters

bool	auto_adjust = true;
bool	gridlines = false;
bool	detail_pane = true;
bool	navigation_pane = false;
bool	preview_pane = false;
bool	loop_cursor = true;
bool	show_hidden_files = false;

int		floating_preview = FP_SELECTION;
int		folderview_flag = 1;
int		folderview_mode = FVM_DETAILS;
int		preview_size_limit = 32 * 1024 * 1024;

WCHAR	navigation_sound[MAX_PATH] = L"AppEvents\\Schemes\\Apps\\Explorer\\Navigating\\.Current";

inline bool SBSP_IS_ABSOLUTE(UINT flags)
{
	return (flags & (SBSP_ABSOLUTE | SBSP_RELATIVE | SBSP_NAVIGATEBACK | SBSP_NAVIGATEFORWARD | SBSP_PARENT)) == SBSP_ABSOLUTE;
}

//==========================================================================================================================

namespace
{
	static HRESULT ExplorerCreate(IServiceProvider* owner, const RECT& bounds, const FOLDERSETTINGS& settings, IExplorerBrowser** ppBrowser, DWORD* ppCookie)
	{
		HRESULT	hr;
		ref<IExplorerBrowser>		browser;
		ref<IExplorerBrowserEvents>	events;
		HWND hwnd = Hwnd::from(owner);
		DWORD cookie = 0;

		if FAILED(hr = browser.newobj(CLSID_ExplorerBrowser))
			return hr;

		// ペインをまったく使用しない場合にはEBO_SHOWFRAMESは不要だが、
		// 以下の不具合があるため常に設定している。
		// ・リストビューの正しくリサイズがされない場合がある。
		// ・コントロールパネルが常にクラシック表示になってしまう。
		browser->SetOptions(EBO_SHOWFRAMES);

		if SUCCEEDED(hr = IUnknown_SetSite(browser, owner))
		{
			if SUCCEEDED(hr = owner->QueryInterface(&events))
				browser->Advise(events, &cookie);
			if SUCCEEDED(hr = browser->Initialize(hwnd, &bounds, &settings))
			{
				*ppBrowser = browser.detach();
				*ppCookie = cookie;
//				browser->SetPropertyBag(PATH_FOLDER_STATE);
				return S_OK;
			}
			// on error
			if (events)
				browser->Unadvise(cookie);
			IUnknown_SetSite(browser, null);
		}
		return hr;
	}

	static void ExplorerDestroy(ref<IExplorerBrowser>& browser, DWORD& cookie)
	{
		if (browser)
		{
			// 一時変数に移しておく
			ref<IExplorerBrowser> _browser(browser);
			DWORD _cookie = cookie;
			browser = null;
			cookie = 0;

			_browser->Unadvise(_cookie);
			IUnknown_SetSite(_browser, null);
			_browser->Destroy();
		}
	}

	static bool ListView_Loop(ListViewH w, UINT dir, UINT vk)
	{
		if (!loop_cursor)
			return false;
		int focus = w.GetNextItem(-1, LVNI_FOCUSED);
		if (focus < 0)
			return false;
		int next = w.GetNextItem(focus, dir);
		if (next >= 0 && next != focus)
			return false;
		w.SendMessage(WM_KEYDOWN, vk, 0);
		return true;
	}

	static HRESULT IFolderView_GetPath(IFolderView* view, int index, WCHAR path[MAX_PATH])
	{
		HRESULT hr;
		ref<IShellFolder> folder;
		ILPtr item;
		SUCCEEDED(hr = view->GetFolder(IID_PPV_ARGS(&folder))) &&
		SUCCEEDED(hr = view->Item(index, &item)) &&
		SUCCEEDED(hr = ILDisplayName(folder, item, SHGDN_FORPARSING, path));
		return hr;
	}

	const PROPERTYKEY* GetColumnsFor(const ITEMIDLIST* item, UINT* count)
	{
		if (ILEquals(item, CSIDL_DRIVES))
		{
			static const PROPERTYKEY COLUMNS_FOR_DRIVES[] =
			{
				PKEY_ItemNameDisplay,	// 名前
				PKEY_ItemTypeText,		// 種類
				PKEY_PercentFull,		// 使用率
				PKEY_FreeSpace,			// 空き領域
				PKEY_Capacity,			// 合計サイズ
				PKEY_Volume_FileSystem,	// ファイルシステム
			};
			*count = lengthof(COLUMNS_FOR_DRIVES);
			return COLUMNS_FOR_DRIVES;
		}
		else if (ILIsDescendant(item, CSIDL_CONTROLS))
		{
			static const PROPERTYKEY COLUMNS_FOR_CONTROLS[] =
			{
				PKEY_ItemNameDisplay,	// 名前
				PKEY_Controls_Category,	// カテゴリ (コントロールパネル専用)
			};
			*count = lengthof(COLUMNS_FOR_CONTROLS);
			return COLUMNS_FOR_CONTROLS;
		}
		else if (ILEquals(item, CSIDL_BITBUCKET))
		{
			static const PROPERTYKEY COLUMNS_FOR_TRASH[] =
			{
				PKEY_ItemNameDisplay,	// 名前
				PKEY_Trash_Where,		// 元の場所
				PKEY_Trash_When,		// 削除日時
				PKEY_Size,				// サイズ
			};
			*count = lengthof(COLUMNS_FOR_TRASH);
			return COLUMNS_FOR_TRASH;
		}
		else if (ILIsDescendant(item, CSIDL_FAVORITES) ||
				 ILIsDescendant(item, CSIDL_LINKS))
		{
			static const PROPERTYKEY COLUMNS_FOR_LINKS[] =
			{
				PKEY_ItemNameDisplay,		// 名前
				PKEY_DateModified,			// 更新日時
				PKEY_Link_TargetParsingPath,// リンク先
			};
			*count = lengthof(COLUMNS_FOR_LINKS);
			return COLUMNS_FOR_LINKS;
		}
		else if (ILIsDescendant(item, CSIDL_GAMES))
		{
			static const PROPERTYKEY COLUMNS_FOR_GAMES[] =
			{
				PKEY_ItemNameDisplay,	// 名前
				PKEY_Game_LastPlay,		// 最終プレイ日
				PKEY_Game_Where,		// インストールの場所
			};
			*count = lengthof(COLUMNS_FOR_GAMES);
			return COLUMNS_FOR_GAMES;
		}
		else
		{
			// 未実装 on Vista
			// PKEY_FileExtension	: ファイル拡張子
			static const PROPERTYKEY COLUMNS_FOR_DIRECTORY[] =
			{
				PKEY_ItemNameDisplay,	// 名前
				PKEY_Size,				// サイズ
				PKEY_DateModified,		// 更新日時
				PKEY_ItemType,			// 種類
#if 0
				PKEY_ComputerName,		// コンピュータ SYNTHIA (このコンピュータ)
				PKEY_DateCreated,		// 作成日時
				PKEY_DateAccessed,		// アクセス日時
				PKEY_FileAttributes,	// 属性
				PKEY_FileName,			// ファイル名
				PKEY_FileOwner,			// 所有者
				PKEY_ItemDate,
				PKEY_ItemFolderNameDisplay,
				PKEY_ItemFolderPathDisplay,
				PKEY_ItemFolderPathDisplayNarrow,
				PKEY_ItemPathDisplay,	// パス
				PKEY_Kind,				// 分類
				PKEY_Rating,			// 評価
#endif
			};
			*count = lengthof(COLUMNS_FOR_DIRECTORY);
			return COLUMNS_FOR_DIRECTORY;
		}
	}
}

//==========================================================================================================================

LRESULT ListView::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		if (m_listview.IsWindow())
			return 0;
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (m_listview)
		{
			MSG m = { m_listview, msg, wParam, lParam };
			ref<IShellView> view;
			if (SUCCEEDED(GetCurrentView(&view)) &&
				view->TranslateAccelerator(&m) == S_OK)
				return 0;
			return m_listview.SendMessage(msg, wParam, lParam);
		}
		break;
	case WM_VSCROLL:
	case WM_HSCROLL:
		return m_listview.SendMessage(msg, wParam, lParam);
	case WM_TIMER:
		switch (wParam)
		{
		case TIMER_PREVIEW:
			KillTimer(TIMER_PREVIEW);
			onPreviewUpdate();
			return 0;
		default:
			if (TIMER_ADJUST <= wParam && wParam < TIMER_ADJUST_MAX)
			{
				KillTimer((UINT)wParam);
				if (!ListView_Adjust(m_listview) && wParam < TIMER_ADJUST_MAX - 1)
					SetTimer((UINT)wParam + 1, DELAY_ADJUST);
				return 0;
			}
			break;
		}
		break;
	case WM_CONTEXTMENU:
		if (!IsParentOf((HWND)wParam))
			return m_defview.SendMessage(msg, wParam, lParam);
		break;
	case SB_SETTEXT:
		SetStatusTextSB((PCWSTR)lParam);
		return true;
	case WM_DESTROY:
		m_listview = null;
		m_defview = null;
		ExplorerDestroy(m_browser, m_cookie);
		SetCurrentFolder(null, true);
		break;
	case WM_SIZE:
		if (m_browser)
			m_browser->SetRect(NULL, get_CenterArea());
		break;
	case WM_CUT:
		cut();
		return 0;
	case WM_COPY:
		copy();
		return 0;
	case WM_PASTE:
		paste();
		return 0;
	case WM_CLEAR:
		remove();
		return 0;
	case WM_UNDO:
		undo();
		return 0;
	case WM_CREATE:
		if (LRESULT lResult = __super::onMessage(msg, wParam, lParam))
			return lResult;
		DisableIME();
		return 0;
	default:
		if (m_listview && LVM_FIRST <= msg && msg <= LVM_LAST)
			return m_listview.SendMessageW(msg, wParam, lParam);
	}
	return __super::onMessage(msg, wParam, lParam);
}

void ListView::onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs)
{
	// WNDCLASSEX
	wc.hCursor = ::LoadCursor(null, IDC_ARROW);
	wc.lpszClassName = L"ListView";
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = ::GetModuleHandle(null);
	// CREATESTRUCT
	cs.dwExStyle = 0;

	m_settings.ViewMode = folderview_mode;
	m_settings.fFlags = FWF_DEFAULT | FWF_SELECT[folderview_flag];

	m_nav = navigation_pane;
	m_detail = detail_pane;
	m_preview = preview_pane;
}

bool ListView::onPress(const MSG& msg)
{
	// リストビュー以外への入力は受け付けない。詳細ビュー編集中など。
	if (m_defview != msg.hwnd && !m_defview.IsParentOf(msg.hwnd))
		return false;

	// 名前編集中は受け入れない。
	if (m_listview.GetEdit())
		return false;

	return __super::onPress(msg);
}

bool ListView::onGesture(const MSG& msg)
{
	// リストビューが消失している場合はすべて受け入れる。
	if (!m_listview.IsWindow())
		return true;

	// 名前編集用のEditでは受け入れない。
	if (msg.hwnd == m_listview.GetEdit())
		return false;

	// ヘッダでは左ダブルクリックは受け入れない。
	if (msg.message == WM_LBUTTONDBLCLK)
	{
		HeaderH header = m_listview.GetHeader();
		if (msg.hwnd == header)
		{
			if (header.HitTest(GET_XY_LPARAM(msg.lParam)) < 0)
			{
				// ヘッダの何もない領域のダブルクリックは幅調整。
				// サブクラス化する手間を省くためジェスチャ内で処理している。
				adjust();
			}
			return false;
		}
	}

	// 左クリックのみのジェスチャは受け入れない。
	if (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP)
		return false;

	LVHITTESTINFO hit = { GET_XY_LPARAM(msg.lParam) };
	if (msg.hwnd != m_listview)
		MapWindowPoints(msg.hwnd, m_listview, &hit.pt, 1);

	if (msg.message == WM_LBUTTONDBLCLK && m_listview.HitTestColumn(0, hit.pt.x))
		return false;	// 名前カラム上は左クリックジェスチャ禁止。

	RECT rc;
	GetClientRect(m_listview, &rc);
	if (hit.pt.x < 0 || hit.pt.y > rc.bottom)
		return false;	// ツリー or 詳細ビュー上のジェスチャ禁止

	int index = m_listview.HitTest(&hit);

	// 右クリックの場合、既に選択済みの項目から開始した場合は、ドラッグとみなす。
	if (msg.message == WM_RBUTTONDOWN &&
		index >= 0 &&
		(hit.flags & LVHT_ONITEM) &&
		m_listview.GetSelectedCount() > 0 &&
		m_listview.GetItemState(index, LVIS_SELECTED) == LVIS_SELECTED)
	{
		return false;
	}

	// アイテム上の左、中クリックはジェスチャしない。
	if (index >= 0 && (MSG2MK(msg) & (MK_LBUTTON | MK_MBUTTON)))
		return false;

	return __super::onGesture(msg);
}

HWND ListView::onFocus() throw()
{
	return m_listview;
}

// @param	x, y	in screen coordinate.
HRESULT ListView::onContextMenu(int x, int y) throw()
{
	// ZIPフォルダ上でメニュー -> 移動 では、OnDefaultCommand() ではなく
	// フォルダ移動を行いたい。そのため、コンテキストメニューは自前で表示する。

	HRESULT hr;
	POINT	ptScreen = { x, y };

	if (ptScreen.x == -1 && ptScreen.y == -1)
	{	// メニューキーの場合の特別
		int index = m_listview.GetNextItem(-1, LVNI_SELECTED);
		if (index < 0)
			return E_FAIL;	// バックグラウンドの場合

		// 選択アイテムの左下に表示する。
		RECT rc = { 0, 0, 0, 0 };
		m_listview.GetItemRect(index, &rc, LVIR_BOUNDS);
		ptScreen.x = rc.left;
		ptScreen.y = rc.bottom;
		m_listview.ClientToScreen(&ptScreen);
	}
	else
	{
		LVHITTESTINFO hit = { ptScreen.x, ptScreen.y };
		m_listview.ScreenToClient(&hit.pt);
		int index = m_listview.HitTest(&hit);
		if (index < 0)
		{
			// バックグラウンドでのメニューは、デフォルトの振る舞いに任せる。
			// 自前で表示させた場合、同じ内容のメニューを出すことまではできるが、
			// 新規作成後、自動的に名前変更状態にさせることが難しいため。
			// メニューに項目を追加するのでない限り、自前で表示させる利点は無い。
			m_listview.SelectItem(-1);
			return E_FAIL;
		}
		else if (m_listview.GetItemState(index, LVNI_SELECTED) == 0)
		{
			// 直下のアイテムを選択する。さもないとメニューが表示されない。
			m_listview.SelectItem(index);
		}
	}

	ref<IShellView> view;
	if FAILED(hr = GetCurrentView(&view))
		return hr;
	hr = MenuPopup(ptScreen, view, this);
	switch (hr)
	{
	case S_DEFAULT:
	{
		ref<IFolderView>	fv;
		ILPtr				item;
		LVHITTESTINFO hit = { ptScreen.x, ptScreen.y };
		m_listview.ScreenToClient(&hit.pt);
		int index = m_listview.HitTest(&hit);
		if (index < 0)
			index = m_listview.GetNextItem(-1, LVNI_FOCUSED);
		if (index < 0)
			index = m_listview.GetNextItem(-1, LVNI_SELECTED);
		if (index >= 0 &&
			SUCCEEDED(view->QueryInterface(&fv)) &&
			SUCCEEDED(fv->Item(index, &item)))
			BrowseObject(item, SBSP_SAMEBROWSER | SBSP_RELATIVE);
		break;
	}
	case S_RENAME:	// 名前変更(rename)時は、UIを独自に提供しなければならない。
		rename();
		break;
	}
	return hr;
}

LRESULT CALLBACK ListView::DefViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	return ((ListView*)data)->DefView_onMessage(msg, wParam, lParam);
}

LRESULT CALLBACK ListView::ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	return ((ListView*)data)->ListView_onMessage(msg, wParam, lParam);
}

namespace
{
	static Hwnd				preview;
	static Hwnd				preview_from;
	static ref<IShellItem>	preview_item;
	static int				preview_index = -1;

	static inline int DistanceOf(const POINT& pt, const RECT& rc) throw()
	{
		int dx, dy;
		if (pt.x < rc.left)
			dx = rc.left - pt.x;
		else if (pt.x > rc.right)
			dx = pt.x - rc.right;
		else
			dx = 0;
		if (pt.y < rc.top)
			dy = rc.top - pt.y;
		else if (pt.y > rc.bottom)
			dy = pt.y - rc.bottom;
		else
			dy = 0;
		return dx + dy;
	}
}

void HidePreview(HWND hwnd)
{
	if (hwnd && hwnd != preview_from)
		return;
	if (preview.IsWindow())
		SetWindowPos(preview, null, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
	preview_from = null;
	preview_item = null;
	preview_index = -1;
}

void ListView::onPreview(int index)
{
	int  MAXDIR = 4;
	switch (get_mode())
	{
	case FVM_THUMBSTRIP:
		index = -1;	// disable iff HUGE mode
		break;
	case FVM_DETAILS:
		MAXDIR = 2;
		break;
	}

	int hot;
	if (index >= 0 &&
		index != (hot = m_listview.GetHotItem()) &&
		m_listview.GetItemState(index, LVNI_SELECTED) == 0)
	{
		if (hot >= 0)
		{
			index = hot;
		}
		else
		{
			POINT pt;
			GetCursorPos(&pt);
			m_listview.ScreenToClient(&pt);
			int hit = m_listview.HitTest(pt.x, pt.y);
			if (hit >= 0 && m_listview.GetItemState(hit, LVNI_FOCUSED | LVNI_SELECTED))
			{
				index = hit;
			}
			else
			{	// マウス直下も選択状態でない。仕方ないので近いアイテムを探す。
				const UINT direction[4] = { LVNI_ABOVE, LVNI_BELOW, LVNI_TOLEFT, LVNI_TORIGHT };
				int min_distance = INT_MAX;
				int min_index = -1;
				for (int i = 0; i < MAXDIR; i++)
				{
					RECT rc;
					int next = m_listview.GetNextItem(index, direction[i] | LVNI_SELECTED);
					if (next >= 0 && next != index && m_listview.GetItemRect(next, &rc, LVIR_BOUNDS))
					{
						int distance = DistanceOf(pt, rc);
						if (distance < min_distance)
						{
							min_distance = distance;
							min_index = next;
						}
					}
				}
				index = min_index;
			}
		}
	}

	if (index >= 0)
	{
		preview_from = m_hwnd;

		ref<IShellItem>		item;

		if SUCCEEDED(GetItemAt(&item, index))
		{
			int	order;
			if (!preview_item || item->Compare(preview_item, SICHINT_CANONICAL, &order) != S_OK)
				SetTimer(TIMER_PREVIEW, DELAY_PREVIEW);
			preview_item = item;
			preview_index = index;
			return;
		}

		HidePreview(null);
	}
	else if (!preview_item || !preview_from.IsWindow() ||
			(preview_index >= 0 &&
			(m_listview.GetHotItem() != preview_index &&
			 m_listview.GetItemState(preview_index, LVNI_SELECTED) == 0)))
	{
		HidePreview(null);
	}
}

void ListView::onPreviewUpdate()
{
	if (floating_preview == FP_DISABLE)
	{
		HidePreview(null);
		return;
	}

	if (preview_from != m_hwnd)
		return;

	if (!preview_item ||
		!IsWindowVisible(m_hwnd) ||
		!IsWindowEnabled(m_hwnd) ||
		IsIconic(GetAncestor(m_hwnd, GA_ROOT)))
	{
		HidePreview(null);
		return;
	}

	// preview_size_limit 以上のファイルはサムネイルを表示しない。
	WIN32_FIND_DATA stat;
	if (SUCCEEDED(PathStat(preview_item, &stat)) &&
		((stat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
		FileSizeOf(stat) > preview_size_limit))
	{
		HidePreview(null);
		return;
	}

	if (BitmapH bitmap = PathThumbnail(preview_item))
	{
		int mode = get_mode();

		UINT LVIR;
		switch (mode)
		{
		case FVM_LIST:
		case FVM_DETAILS:
			LVIR = LVIR_LABEL;
			break;
		default:
			LVIR = LVIR_BOUNDS;
			break;
		}

		RECT rc;
		m_listview.GetItemRect(preview_index, &rc, LVIR);
		POINT pt = (LVIR == LVIR_LABEL ? RECT_NE(rc) : RECT_SW(rc));
		m_listview.ClientToScreen(&pt);

		if (!preview.IsWindow())
		{
			preview = CreateWindowEx(
				WS_EX_LAYERED, WC_STATIC, NULL, WS_POPUP | SS_BITMAP,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				::GetWindow(), null, ::GetModuleHandle(null), NULL);
		}

		preview.SetLayerBitmap(bitmap, PREVIEW_ALPHA);
		SIZE sz = BitmapSize(bitmap);
		RECT area;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &area, 0);
		if (pt.x < area.top)
			pt.x = area.top;
		if (pt.y < area.left)
			pt.y = area.left;
		if (pt.x + sz.cx > area.right)
		{
			if (LVIR == LVIR_LABEL)
				pt.x = pt.x - RECT_W(rc) - sz.cx - 16;
			else
				pt.x = area.right - sz.cx;
		}
		if (pt.y + sz.cy > area.bottom)
		{
			if (LVIR == LVIR_LABEL)
				pt.y = area.bottom - sz.cy;
			else
				pt.y = pt.y - RECT_H(rc) - sz.cy;
		}
		SetWindowPos(preview, null, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
	else
	{
		HidePreview(null);
	}
}

LRESULT CALLBACK ListView::DirectUIHWNDProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	return ((ListView*)data)->DirectUIHWND_onMessage(msg, wParam, lParam);
}

LRESULT	ListView::DefView_onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->code)
		{
		case LVN_BEGINLABELEDIT:
			DefView_onBeginLabelEdit((NMLVDISPINFO*)lParam);
			break;
		case LVN_HOTTRACK:
			if (floating_preview >= FP_HOTTRACK)
				onPreview(((NMLISTVIEW*)lParam)->iItem);
			break;
		case LVN_ITEMCHANGED:
			if (floating_preview >= FP_SELECTION)
				onPreview(((NMLISTVIEW*)lParam)->iItem);
			break;
		}
		break;
	case WM_CONTEXTMENU:
		if SUCCEEDED(onContextMenu(GET_XY_LPARAM(lParam)))
			return 0;
		break;
	case WM_DESTROY:
		::RemoveWindowSubclass(m_defview, DefViewProc, 0);
		break;
	}
	return ::DefSubclassProc(m_defview, msg, wParam, lParam);
}

void ListView::DefView_onBeginLabelEdit(NMLVDISPINFO* info)
{
	Hwnd edit = m_listview.GetEdit();
	if (WINDOWS_VERSION < WINDOWS_VISTA)
	{	// XPのみ。Vista では拡張子を選択しないのがデフォルト。
		WCHAR path[MAX_PATH];
		ref<IFolderView> view;
		if (SUCCEEDED(GetCurrentView(&view)) &&
			SUCCEEDED(IFolderView_GetPath(view, info->item.iItem, path)) &&
			!PathIsDirectory(path))
		{
			WCHAR name[MAX_PATH];
			edit.GetText(name, MAX_PATH);
			PCWSTR ext = ::PathFindExtension(name);
			int base = (int)(ext - name);
			if (base > 0)
				edit.PostMessage(EM_SETSEL, 0, base); // must be Post
		}
	}
	SubclassSingleLineEdit(sq::vm, edit);
}

LRESULT	ListView::ListView_onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SETFOCUS:
		set_focus(m_listview);
		break;
	case WM_LBUTTONDOWN:
		if ((GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT) &&
			m_listview.GetNextItem(-1, LVNI_SELECTED) < 0)
		{
			// 最初にシフト＋左クリックすると、一番上からクリックした位置まで選択される。
			// 直感に反するため、シフトが押されていないものとして扱う。
			wParam = MAKEWPARAM(LOWORD(wParam) & ~MK_SHIFT, HIWORD(wParam));
		}
		break;
	case WM_MBUTTONDOWN:
	{
		int index = m_listview.HitTest(GET_XY_LPARAM(lParam));
		if (index >= 0)
		{
			if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL)
			{
				m_listview.SetItemState(index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
				m_listview.EnsureVisible(index);
			}
			else
				m_listview.SelectItem(index);
		}
		return 0;
	}
	case WM_MBUTTONUP:
	{
		int index = m_listview.HitTest(GET_XY_LPARAM(lParam));
		if (index >= 0 && m_listview.GetItemState(index, LVIS_SELECTED))
			press_with_mods(VK_RETURN, GET_KEYSTATE_WPARAM(wParam) | MK_MBUTTON);
		return 0;
	}
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_UP:
			if (ListView_Loop(m_listview, LVNI_ABOVE, VK_END))
				return 0;
			break;
		case VK_DOWN:
			if (ListView_Loop(m_listview, LVNI_BELOW, VK_HOME))
				return 0;
			break;
		case VK_SPACE: // 単にスペースが押されたら次のアイテムへ移動
			if (m_listview.GetSelectedCount() == 1 && !(m_listview.GetExtendedStyle() & LVS_EX_CHECKBOXES))
			{
				int	count = m_listview.GetItemCount();
				if (count > 1)
				{
					int	index =  m_listview.GetNextItem(-1, LVNI_FOCUSED);
					if (index == m_listview.GetNextItem(-1, LVNI_SELECTED))
					{
						int	next;
						if (IsKeyPressed(VK_SHIFT))
							next = (index > 0 ? index - 1 : count - 1);
						else
							next = (index < count - 1 ? index + 1 : 0);
						m_listview.SetItemState(index, 0, LVIS_SELECTED);
						m_listview.SetItemState(next, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
						m_listview.EnsureVisible(next);
						return 0;
					}
				}
			}
			break;
		}
		break;
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		HidePreview(m_hwnd);
		break;
	case WM_DESTROY:
		HidePreview(m_hwnd);
		::RemoveWindowSubclass(m_listview, ListViewProc, 0);
		break;
	}
	return ::DefSubclassProc(m_listview, msg, wParam, lParam);
}

LRESULT	ListView::DirectUIHWND_onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MOUSEWHEEL:
		if (int n = m_listview.GetItemCount())
		{
			int i = m_listview.GetNextItem(-1, LVNI_FOCUSED);
			if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
			{
				if (++i >= n)
					i = 0;
			}
			else
			{
				if (--i < 0)
					i = n - 1;
			}
			m_listview.SelectItem(i);
		}
		return 0;
	}
	return ::DefSubclassProc(m_frame, msg, wParam, lParam);
}

HRESULT ListView::GetCurrentFolder(REFINTF pp) const throw()
{
	HRESULT hr;
	ref<IFolderView> view;
	if FAILED(hr = GetCurrentView(&view))
		return hr;
	return view->GetFolder(pp.iid, pp.pp);
}

HRESULT ListView::GetCurrentView(REFINTF pp) const throw()
{
	if (!m_browser)
	{
		*pp.pp = null;
		return E_UNEXPECTED;
	}
	return m_browser->GetCurrentView(pp.iid, pp.pp);
}

HRESULT ListView::GetItemAt(IShellItem** pp, int index) const throw()
{
	ref<IFolderView2>	fv2;
	if SUCCEEDED(GetCurrentView(&fv2))
		return fv2->GetItem(index, IID_PPV_ARGS(pp));

	ref<IFolderView>	fv;
	ILPtr				leaf;
	if (SUCCEEDED(GetCurrentView(&fv)) &&
		SUCCEEDED(fv->Item(index, &leaf)))
		return PathCreate(pp, null, m_path, leaf);

	return E_NOTIMPL;
}

void ListView::SetCurrentFolder(const ITEMIDLIST* path, bool copy) throw()
{
	if (path)
		m_path.attach(copy ? ILClone(path) : const_cast<ITEMIDLIST*>(path));
	else
		m_path.clear();
}

void ListView::DoPress(UINT vk, UINT mk)
{
	ref<IShellView> view;
	if (m_listview && SUCCEEDED(GetCurrentView(&view)))
	{
		MSG msg = { m_listview, WM_KEYDOWN, vk, 0 };
		SetKeyState(vk, mk);
		view->TranslateAccelerator(&msg);
		ResetKeyState(vk);
	}
}

//==========================================================================================================================
// IServiceProvider

IFACEMETHODIMP ListView::QueryService(REFGUID guidService, REFIID riid, void **pp)
{
	if (guidService == SID_SExplorerBrowserFrame ||
		guidService == SID_SShellBrowser ||
		guidService == SID_ExplorerPaneVisibility)
	{
		return QueryInterface(riid, pp);
	}
	else if (!pp)
		return E_POINTER;
	else
	{
		*pp = null;
		return E_NOINTERFACE;
	}
}

//==========================================================================================================================
// IExplorerBrowserEvents

IFACEMETHODIMP ListView::OnNavigationPending(const ITEMIDLIST* path)
{
	// Returning failure from this will cancel the navigation. 
	return S_OK;
}

IFACEMETHODIMP ListView::OnViewCreated(IShellView* view)
{
	// Called once the view window has been created.
	// Do any last minute modifcations to the view here before it is shown
	// (set view modes, folder flags, etc...)
	RestoreFolderSettings();

	if (HWND hwnd = Hwnd::from(view))
	{
		if (m_defview != hwnd)
		{
			// Attach DefView
			if (m_defview.IsWindow())
				::RemoveWindowSubclass(m_defview, DefViewProc, 0);
			m_defview = hwnd;
			::SetWindowSubclass(m_defview, DefViewProc, 0, (DWORD_PTR)this);
			// Attach ListView
			if (m_listview.IsWindow())
				::RemoveWindowSubclass(m_listview, ListViewProc, 0);
			m_listview = ::FindWindowEx(m_defview, null, WC_LISTVIEW, null);
			if (m_listview)
			{
				::SetWindowSubclass(m_listview, ListViewProc, 0, (DWORD_PTR)this);
				m_listview.DisableIME();
				if (HFONT font = GetFont())
					m_listview.SetFont(font);
				SetExtendedStyle(LVS_EX_GRIDLINES, gridlines);
				if (WINDOWS_VERSION < WINDOWS_VISTA)
					SetExtendedStyle(LVS_EX_FULLROWSELECT, (FWF_SELECT[folderview_flag] & FWF_FULLROWSELECT) != 0);
			}
			// Attach Preview
			m_frame = FindWindowByClass(m_hwnd, L"DirectUIHWND");
			if (m_frame)
				::SetWindowSubclass(m_frame, DirectUIHWNDProc, 0, (DWORD_PTR)this);
		}
	}

	ILPtr				item;
	ref<IColumnManager>	columns;
	if (SUCCEEDED(view->QueryInterface(&columns)) &&
		(item = ILCreate(view)) != null)
	{
		const PROPERTYKEY*	props;
		UINT				numProps;
		if ((props = GetColumnsFor(item, &numProps)) != null && numProps > 0)
			columns->SetColumns(props, numProps);
	}

	return S_OK;
}

static void PlayNavigateSound(HWND hwnd)
{
	if (!str::empty(navigation_sound) && ::IsWindowVisible(hwnd))
		Sound(navigation_sound);
}

// Called once the navigation has succeeded (after OnViewCreated).
IFACEMETHODIMP ListView::OnNavigationComplete(const ITEMIDLIST* path)
{
	// 初めての移動の場合には鳴らさない。
	if (m_path)
		PlayNavigateSound(m_hwnd);

	SetCurrentFolder(path, true);
	// いろいろな場所で呼ぶ必要がある？ とりあえずここで呼べば設定を引き継げる。
	RestoreFolderSettings();
	// TODO: XP の場合は、親方向への移動で以前いたフォルダを選択する
	onEvent(L"onNavigate");

	adjust_with_delay();
	return S_OK;
}

IFACEMETHODIMP ListView::OnNavigationFailed(const ITEMIDLIST* path)
{
	// Called if a navigation failed, despite the call to IShellBrowser::BrowseObject succeeding.
	if (ILIsEqual(m_path, path))
	{
		ref<IPersistIDList>	persist;
		ITEMIDLIST*			current;	// 所有権を引き継ぐので解放しない。
		if (SUCCEEDED(GetCurrentFolder(&persist)) &&
			SUCCEEDED(persist->GetIDList(&current)))
			SetCurrentFolder(current, false);
	}
	return S_OK;
}

//==========================================================================================================================
// IExplorerPaneVisibility

IFACEMETHODIMP ListView::GetPaneState(REFEXPLORERPANE ep, EXPLORERPANESTATE *peps)
{
//	EP_Commands,
//	EP_Commands_Organize,
//	EP_Commands_View,

	if ((m_nav && ep == EP_NavPane) ||
		(m_detail && ep == EP_DetailsPane) ||
		(m_preview && ep == EP_PreviewPane))
	{
		*peps = EPS_DEFAULT_ON | EPS_FORCE;
		return S_OK;
	}

	*peps = EPS_DEFAULT_OFF | EPS_FORCE;
	return S_OK;
}

//==========================================================================================================================
// IOleWindow

IFACEMETHODIMP ListView::GetWindow(HWND* phwnd)
{
	if (!phwnd)
		return E_POINTER;
	*phwnd = m_hwnd;
	return S_OK;
}

//==========================================================================================================================
// ICommDlgBrowser

// handle double click and ENTER key if needed
IFACEMETHODIMP ListView::OnDefaultCommand(IShellView* view)
{
	// 更新しておく
	view->GetCurrentInfo(&m_settings);

	ref<IShellItemArray> items;
	if FAILED(PathsCreate(&items, view, SVGIO_SELECTION | SVGIO_FLAG_VIEWORDER))
		return S_FALSE;

	// イベントハンドラで処理された場合には、処理を打ち切る。
	if (onEvent(L"onExecute", items.ptr))
		return S_OK;

	// .zip などのデフォルトがフォルダ移動ではない表示可能なフォルダは
	// S_FALSE を返すだけでは移動してくれないため自前で移動する。
	ref<IShellItem> item;
	if (SUCCEEDED(items->GetItemAt(0, &item)) &&
		PathBrowsable(item) == S_OK &&
		SUCCEEDED(BrowseObject(ILCreate(item), SBSP_SAMEBROWSER | SBSP_ABSOLUTE)))
			return S_OK;
	return S_FALSE;
}

IFACEMETHODIMP ListView::OnStateChange(IShellView* view, ULONG uChange)
{	//handle selection, rename, focus if needed
	switch (uChange)
	{
	case CDBOSC_SETFOCUS:
	case CDBOSC_KILLFOCUS:
	case CDBOSC_SELCHANGE:
	case CDBOSC_RENAME:
	case CDBOSC_STATECHANGE:
		break;
	}
	return E_NOTIMPL;
}

IFACEMETHODIMP ListView::IncludeObject(IShellView* view, const ITEMIDLIST* pidl)
{	// 表示ファイルのフィルタリング。
	// S_OK: show, S_FALSE: hide
	return S_OK;
}

//==========================================================================================================================
// ICommDlgBrowser2

IFACEMETHODIMP ListView::GetDefaultMenuText(IShellView* view, WCHAR* pszText, int cchMax)
{
	ref<IFolderView>	fv;
	ref<IShellFolder>	sf;
	ITEMIDLIST*			item = null;
	SFGAOF				flags = SFGAO_BROWSABLE | SFGAO_FOLDER;
	int					focus = m_listview.GetNextItem(-1, LVNI_FOCUSED);

	if (focus >= 0 &&
		SUCCEEDED(view->QueryInterface(&fv)) &&
		SUCCEEDED(fv->GetFolder(IID_PPV_ARGS(&sf))) &&
		SUCCEEDED(fv->Item(focus, &item)) &&
		SUCCEEDED(sf->GetAttributesOf(1, (const ITEMIDLIST**)&item, &flags)) &&
		(flags & (SFGAO_BROWSABLE | SFGAO_FOLDER)))
	{
		str::load(STR_GO, pszText, cchMax);
	}
	else
	{
		pszText[0] = 0;
	}
	ILFree(item);
	return S_OK;
}

IFACEMETHODIMP ListView::Notify(IShellView* view, DWORD dwNotifyType)
{
	switch (dwNotifyType)
	{
	case CDB2N_CONTEXTMENU_START:
	case CDB2N_CONTEXTMENU_DONE:
		break;
	}
	return E_NOTIMPL;
}

IFACEMETHODIMP ListView::GetViewFlags(DWORD* pdwFlags)
{	// すべてのファイルを表示すべきか否かの問い合わせ。
	// CDB2GVF_SHOWALLFILES: 隠しファイルを表示する。
	// 0: 隠しファイルを表示しない。
	if (show_hidden_files)
		*pdwFlags = CDB2GVF_SHOWALLFILES;
	else
		*pdwFlags = 0;
	return S_OK;
}

//==========================================================================================================================
// IShellBrowser

IFACEMETHODIMP ListView::SetStatusTextSB(PCWSTR text)
{
	// ステータスバーへの表示はこの関数に集約する。
	// ただしVistaでは送られてこない。
	return S_OK;
}

IFACEMETHODIMP ListView::OnViewWindowActive(IShellView* pShellView)
{
	// WM_SETFOCUS と同義。
	return S_OK;
}

// Vistaではステータスバーメッセージは送られてこないようだ。
IFACEMETHODIMP ListView::GetControlWindow(UINT id, HWND* pHwnd)
{
#if NOT_USED
	if (!pHwnd)
		return E_POINTER;
	switch (id)
	{
	case FCW_STATUS:
		*pHwnd = m_hwnd;
		return S_OK;
	}
#endif
	return E_NOTIMPL;
}

IFACEMETHODIMP ListView::SendControlMsg(UINT id, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* lResult)
{
#if NOT_USED
	if (id != FCW_STATUS || msg != SB_SETTEXT)
		return E_NOTIMPL;
	SetStatusTextSB((PCWSTR)lParam);
	if (lResult)
		*lResult = TRUE;
	return S_OK;
#else
	return E_NOTIMPL;
#endif
}

IFACEMETHODIMP ListView::QueryActiveShellView(IShellView** pp)
{
	return GetCurrentView(pp);
}

IFACEMETHODIMP ListView::BrowseObject(const ITEMIDLIST* pidl, UINT flags)
{
	HRESULT	hr;

	if (!IsWindow())
		return E_UNEXPECTED;

	// 同じディレクトリへの移動の場合は何もしない。
	if (SBSP_IS_ABSOLUTE(flags) && ILIsEqual(m_path, pidl))
		return S_FALSE;

	HidePreview(m_hwnd);

	if (m_browser)
	{
		SaveFolderSettings();
		return m_browser->BrowseToIDList(pidl, flags);
	}

	if (!SBSP_IS_ABSOLUTE(flags))
		return E_UNEXPECTED;

	// 初回呼び出しの場合。
	m_settings.ViewMode = folderview_mode;
	if FAILED(hr = ExplorerCreate(this, get_CenterArea(), m_settings, &m_browser, &m_cookie))
		return hr;
	if FAILED(hr = m_browser->BrowseToIDList(pidl, SBSP_ABSOLUTE | SBSP_SAMEBROWSER))
	{
		ExplorerDestroy(m_browser, m_cookie);
		return hr;
	}

	return S_OK;
}

//==========================================================================================================================

ref<IShellItem>	ListView::get_path() const
{
	ref<IShellItem> value;
	PathCreate(&value, m_path);
	return value;
}

void ListView::set_path(std::pair<PCWSTR, IShellItem*> value)
{
	HRESULT hr;
	if (value.first)
	{
		PCWSTR s = value.first;
		if (wcscmp(s, L"..") == 0)
			hr = BrowseObject(NULL, SBSP_SAMEBROWSER | SBSP_PARENT);
		else if (wcscmp(s, L"<") == 0)
			hr = BrowseObject(NULL, SBSP_SAMEBROWSER | SBSP_NAVIGATEBACK);
		else if (wcscmp(s, L">") == 0)
			hr = BrowseObject(NULL, SBSP_SAMEBROWSER | SBSP_NAVIGATEFORWARD);
		else
			hr = BrowseObject(ILCreate(s), SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
	}
	else
	{
		hr = BrowseObject(ILCreate(value.second), SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
	}
	if FAILED(hr)
		throw sq::Error(hr, L"ListView.set_path()");
}

/*
void sq::push(HSQUIRRELVM v, IEnumIDList* value) throw()
{
	if (value)
	{
		sq_newarray(v, 0);
		ITEMIDLIST* pidl;
		while (value->Next(1, &pidl, NULL) == S_OK)
		{
			ref<IShellItem> item;
			if SUCCEEDED(PathCreate(&item, pidl))
			{
				sq::push(v, item);
				sq_arrayappend(v, -2);
			}
			ILFree(pidl);
		}
	}
	else
		sq_pushnull(v);
}
ref<IEnumIDList> ListView::get_items() const
{
	ref<IEnumIDList>	items;

	ref<IFolderView> fv;
	if (SUCCEEDED(GetCurrentView(&fv)) &&
		SUCCEEDED(fv->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(&items))))
		return items;

	return null;
}
*/

ref<IShellItemArray> ListView::get_selection() const
{
	ref<IShellItemArray>	items;

	if (m_listview.GetSelectedCount() == 0)
		return null;

	// 表示順で取得したいので IShellView を優先して使う.
	ref<IShellView> view;
	if (SUCCEEDED(GetCurrentView(&view)) &&
		SUCCEEDED(PathsCreate(&items, view, SVGIO_SELECTION | SVGIO_FLAG_VIEWORDER)))
		return items;

	// IFolderView2.GetSelection() だと選択アイテムが先頭になってしまう.
	ref<IFolderView2> fv2;
	if (SUCCEEDED(GetCurrentView(&fv2)) &&
		SUCCEEDED(fv2->GetSelection(FALSE, &items)))
		return items;

	return null;
}

static void ListView_SelectChecked(ListViewH v, UINT state)
{
	int		count = v.GetItemCount();
	for (int i = 0; i < count; ++i)
	{
		UINT	checked = ListView_GetCheckState(v, i);
		v.SetItemState(i, ((checked & state) ? LVIS_SELECTED : 0), LVIS_SELECTED);
	}
}

SQInteger ListView::set_selection(sq::VM v)
{
	set_selection_svsi(v, 2, SVSI_SELECT | SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS);
	return 0;
}

void ListView::set_selection_svsi(sq::VM v, SQInteger idx, UINT svsi)
{
	switch (sq_gettype(v, idx))
	{
	case OT_NULL:
		m_listview.SelectItem(-1);
		break;
	case OT_INTEGER:
	{
		int value = v.get<int>(idx);
		switch (value)
		{
		case ALL:
			m_listview.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
			break;
		case CHECKED:
			ListView_SelectChecked(m_listview, 0x3);
			break;
		case CHECKED1:
			ListView_SelectChecked(m_listview, 0x1);
			break;
		case CHECKED2:
			ListView_SelectChecked(m_listview, 0x2);
			break;
		default:
			m_listview.SelectItem(value);
			break;
		}
		break;
	}
	case OT_STRING:
	{	// by name or wildcard
		PCWSTR pattern = v.get<PCWSTR>(idx);

		bool (*match)(PCWSTR pattern, PCWSTR text_);
		if (StrPBrk(pattern, L"*?"))
			match = MatchPatternI;
		else
			match = MatchContainsI;

		int		ln = m_listview.GetItemCount();
		for (int i = 0; i < ln; ++i)
		{
			WCHAR text_[MAX_PATH];
			if (!m_listview.GetItemText(i, 0, text_, MAX_PATH))
				continue;
			if (!match(pattern, text_))
				continue;
			ListView_SelectItem_SVSI(m_listview, i, svsi);
			svsi &= ~(SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS);
		}
		break;
	}
	case OT_CLOSURE:
	{	// by filter (callable)
		int		ln = m_listview.GetItemCount();
		for (int i = 0; i < ln; ++i)
		{
			WCHAR text[MAX_PATH];
			if (!m_listview.GetItemText(i, 0, text, MAX_PATH))
				continue;
			sq_push(v, idx);
			v.push(get_self());
			v.push(text);
			sq::call(v, 2, SQTrue);
			if (!v.get<bool>(-1))
				continue;
			ListView_SelectItem_SVSI(m_listview, i, svsi);
			svsi &= ~(SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS);
		}
		break;
	}
	case OT_USERDATA:
	{	// by IShellItem
		IShellItem* item;
		sq::get(v, idx, &item);
		ref<IShellView> view;
		if SUCCEEDED(GetCurrentView(&view))
		{
			ILPtr child = ILCreate(item);
			if (ILIsParent(m_path, child, false))
				view->SelectItem((ITEMIDLIST*)((BYTE*)child.ptr + ILGetSize(m_path) - 2), svsi);
		}
		break;
	}
	case OT_INSTANCE:
	{	// by re object
		v.getslot(2, L"match");

		int		ln = m_listview.GetItemCount();
		for (int i = 0; i < ln; ++i)
		{
			WCHAR text[MAX_PATH];
			if (!m_listview.GetItemText(i, 0, text, MAX_PATH))
				continue;
			sq_push(v, -1);		// match()
			sq_push(v, idx);	// this
			v.push(text);		// text
			sq::call(v, 2, SQTrue);
			if (!v.get<bool>(-1))
				continue;
			ListView_SelectItem_SVSI(m_listview, i, svsi);
			svsi &= ~(SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS);
		}
		break;
	}
	case OT_ARRAY:
	{
		ref<IShellView> view;
		if SUCCEEDED(GetCurrentView(&view))
		{
			foreach (v, idx)
			{
				set_selection_svsi(v, v.gettop(), svsi);
				svsi &= ~(SVSI_FOCUSED | SVSI_DESELECTOTHERS);
			}
		}
		break;
	}
	default:
		throw sq::Error(L"ListView.set_selection not supported");
	}
}

ref<IShellItem> ListView::item(int index) const
{
	ref<IShellItem>		item;
	ref<IFolderView2>	fv2;
	// まずは IFolderView2 を試す
	if SUCCEEDED(GetCurrentView(&fv2))
	{
		if SUCCEEDED(fv2->GetItem(index, IID_PPV_ARGS(&item)))
			return item;
	}
	// ダメなら IFolderView を試す
	ref<IFolderView>	fv = fv2;
	if (fv || SUCCEEDED(GetCurrentView(&fv)))
	{
		ILPtr	pidl;
		if (SUCCEEDED(fv->Item(index, &pidl)) &&
			SUCCEEDED(PathCreate(&item, pidl)))
			return item;
	}
	return null;
}

namespace sq
{
	class ThrowOnError
	{
	public:
		void operator = (HRESULT hr)
		{
			if FAILED(hr)
				throw sq::Error(L"HRESULT: 0x08X", hr);
		}
	};
}

static PCWSTR key2str(WCHAR buffer[], size_t len, const PROPERTYKEY& key)
{
	swprintf_s(buffer, len,
		L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}:%lu",
		key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
		key.fmtid.Data4[0], key.fmtid.Data4[1],
		key.fmtid.Data4[2], key.fmtid.Data4[3],
		key.fmtid.Data4[4], key.fmtid.Data4[5],
		key.fmtid.Data4[6], key.fmtid.Data4[7],
		key.pid);
	return buffer;
}

static bool str2key(const PCWSTR buffer, PROPERTYKEY* key)
{
	return 12 == swscanf_s(buffer,
		L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}:%lu",
		&key->fmtid.Data1, &key->fmtid.Data2, &key->fmtid.Data3,
		&key->fmtid.Data4[0], &key->fmtid.Data4[1],
		&key->fmtid.Data4[2], &key->fmtid.Data4[3],
		&key->fmtid.Data4[4], &key->fmtid.Data4[5],
		&key->fmtid.Data4[6], &key->fmtid.Data4[7],
		&key->pid);
}

SQInteger ListView::get_columns(sq::VM v)
{
	// CM_ENUM_ALL or CM_ENUM_VISIBLE

	sq::ThrowOnError			hr;
	ref<IColumnManager>			columns;
	UINT						count;
	std::vector<PROPERTYKEY>	keys;

	hr = GetCurrentView(&columns);
	hr = columns->GetColumnCount(CM_ENUM_VISIBLE, &count);
	keys.resize(count);
	hr = columns->GetColumns(CM_ENUM_VISIBLE, &keys[0], count);

	v.newarray();

	CM_COLUMNINFO col = { sizeof(col), CM_MASK_WIDTH | CM_MASK_NAME };
	for (UINT i = 0; i < count; ++i)
	{
		WCHAR	strkey[64];
		hr = columns->GetColumnInfo(keys[i], &col);

		v.newtable();
		// name
		v.push(L"key");
		v.push(key2str(strkey, lengthof(strkey), keys[i]));
		v.newslot(-3);
		// name
		v.push(L"name");
		v.push(col.wszName);
		v.newslot(-3);
		// width
		v.push(L"width");
		v.push(col.uWidth);
		v.newslot(-3);

		v.append(-2);
	}

	return 1;
}

SQInteger ListView::set_columns(sq::VM v)
{
	sq::ThrowOnError			hr;
	ref<IColumnManager>			columns;
	std::vector<PROPERTYKEY>	keys;

	hr = GetCurrentView(&columns);

	foreach (v, 1)
	{
		PROPERTYKEY	key;
		PCWSTR		str;

		if (sq_gettype(v, -1) == OT_STRING)
		{
			str = v.get<PCWSTR>(-1);
		}
		else
		{
			v.getslot(-1, L"key");
			str = v.tostring(-1);
		}

		if (!str2key(str, &key))
			throw sq::Error(L"invalid key format: %s", str);
		keys.push_back(key);
	}

	if (!keys.empty())
		hr = columns->SetColumns(&keys[0], (UINT)keys.size());

	return 0;
}


void ListView::SaveFolderSettings() const throw()
{
	ref<IShellView> view;
	if SUCCEEDED(GetCurrentView(&view))
	{
		view->GetCurrentInfo(&m_settings);
		switch (m_settings.ViewMode)
		{
		case FVM_LIST:
		case FVM_DETAILS:
		case FVM_TILE:
			return;	// ok
		}
		ref<IFolderView2> fv2;
		int	size;
		if (SUCCEEDED(view->QueryInterface(&fv2)) &&
			SUCCEEDED(fv2->GetViewModeAndIconSize((FOLDERVIEWMODE*)&m_settings.ViewMode, &size)))
		{
			if (size > 96)
				m_settings.ViewMode = FVM_HUGE;
			else if (size > 48)
				m_settings.ViewMode = FVM_LARGE;
			else if (size > 16)
				m_settings.ViewMode = FVM_MEDIUM;
			else
				m_settings.ViewMode = FVM_SMALL;
		}
	}
}

void ListView::RestoreFolderSettings() throw()
{
	// TODO: アイコンサイズをカスタマイズするオプションを追加する。
	// 例えば、タイル表示であっても 48x48 以外のサイズを使って良い。
	const int ICONSIZE[] = { 48, 16, 16, 16, 96, 48, 256 };

	if (m_browser)
		m_browser->SetFolderSettings(&m_settings);
	ref<IFolderView2> fv2;
	if SUCCEEDED(GetCurrentView(&fv2))
		fv2->SetViewModeAndIconSize((FOLDERVIEWMODE)m_settings.ViewMode, ICONSIZE[m_settings.ViewMode-1]);
}

bool ListView::get_sorting() const
{
	SaveFolderSettings();
	return (m_settings.fFlags & FWF_AUTOARRANGE) != 0;
}

void ListView::set_sorting(bool value)
{
	SaveFolderSettings();
	if (value)
		m_settings.fFlags |= FWF_AUTOARRANGE;
	else
		m_settings.fFlags &= ~FWF_AUTOARRANGE;
	RestoreFolderSettings();
}

int ListView::get_mode() const
{
	SaveFolderSettings();
	return m_settings.ViewMode;
}

void ListView::set_mode(int value)
{
	SaveFolderSettings();
	if (FVM_FIRST <= value && value <= FVM_LAST && m_settings.ViewMode != (UINT)value)
	{
		m_settings.ViewMode = value;
		RestoreFolderSettings();
	}
}

bool ListView::get_nav() const
{
	return m_nav;
}

void ListView::set_nav(bool value)
{
	if (m_nav != value)
	{
		m_nav = value;
		recreate();
	}
}

bool ListView::get_detail() const
{
	return m_detail;
}

void ListView::set_detail(bool value)
{
	if (m_detail != value)
	{
		m_detail = value;
		recreate();
	}
}

bool ListView::get_preview() const
{
	return m_preview;
}

void ListView::set_preview(bool value)
{
	if (m_preview != value)
	{
		m_preview = value;
		recreate();
	}
}

void ListView::rename()
{
	if (!m_listview)
		return;

	// フォーカス中の項目が選択されていない場合、最初の選択項目をフォーカスする。
	// 複数項目のリネームの場合に、直感に反する動作をするため。
	int focus = m_listview.GetNextItem(-1, LVNI_FOCUSED);
	if (focus >= 0 && !m_listview.GetItemState(focus, LVIS_SELECTED))
	{
		int selected = m_listview.GetNextItem(-1, LVNI_SELECTED);
		if (selected >= 0)
			m_listview.SetItemState(selected, LVIS_FOCUSED, LVIS_FOCUSED);
	}

	// IFolderView2 ならば専用のメソッドがある。
	ref<IFolderView2> view;
	if (SUCCEEDED(GetCurrentView(&view)) && SUCCEEDED(view->DoRename()))
		return;
	// IFolderView には無いので、F2 キーを押すエミュレーションを行う。
	DoPress(VK_F2);
}

int ListView::len() const throw()
{
	ref<IFolderView> view;
	int count;
	if (SUCCEEDED(GetCurrentView(&view)) &&
		SUCCEEDED(view->ItemCount(SVGIO_ALLVIEW, &count)))
		return count;
	else if (m_listview)
		return m_listview.GetItemCount();
	else
		return 0;
}

void ListView::adjust_with_delay()
{
	if (auto_adjust && m_listview)
		SetTimer(TIMER_ADJUST, DELAY_ADJUST);
}

void ListView::adjust()	{ ListView_Adjust(m_listview); }
void ListView::cut()	{ DoPress('X', MK_CONTROL); }
void ListView::copy()	{ DoPress('C', MK_CONTROL); }
void ListView::paste()	{ DoPress('V', MK_CONTROL); }
void ListView::undo()	{ DoPress('Z', MK_CONTROL); }

void ListView::remove()
{
	if (!m_path)
		return;

	bool	doBury = false;
	WCHAR	dir[MAX_PATH];
	if SUCCEEDED(ILPath(m_path, dir))
	{
		if (PathIsUNC(dir))
		{
			// たぶんネットワークパス
			doBury = true;
		}
		else if (PathStripToRoot(dir))
		{
			switch (GetDriveType(dir))
			{
			case DRIVE_REMOVABLE:
			case DRIVE_REMOTE:
			case DRIVE_CDROM:
			case DRIVE_RAMDISK:
				// ごみ箱が使えない場合は、完全削除扱いにする。
				doBury = true;
				break;
			}
		}
	}
	if (doBury)
		bury();
	else
		DoPress(VK_DELETE);
}

void ListView::bury()
{
	if (!m_path)
		return;
	DoPress(VK_DELETE, MK_SHIFT);
}

void ListView::newfile()
{
	ref<IShellView> view;
	if SUCCEEDED(GetCurrentView(&view))
		FileNew(view, false);
}

void ListView::newfolder()
{
	ref<IShellView> view;
	if SUCCEEDED(GetCurrentView(&view))
		FileNew(view, true);
}

namespace
{
	static void DoRefresh(IShellView* sv, HWND lv)
	{
		// 状態を保存する
		sv->SaveViewState();

		ref<IFolderView2> fv2;
		if SUCCEEDED(sv->QueryInterface(&fv2))
		{
			// ソート状態の保存
			std::vector<SORTCOLUMN> columns;
			int n = 0;
			if (SUCCEEDED(fv2->GetSortColumnCount(&n)) && n > 0)
			{
				columns.resize(n);
				if SUCCEEDED(fv2->GetSortColumns(&columns[0], n))
				{
					// 更新
					sv->Refresh();
					// ソート状態の復元
					fv2->SetSortColumns(&columns[0], n);
					return;
				}
			}
		}

		// IFolderView2 での処理が失敗したならば、ListVide で代替する。

		// ソート状態の保存
		int		column = 0;
		bool	ascending = true;
		ListView_GetSortedColumn(lv, &column, &ascending);
		// 更新
		sv->Refresh();
		// ソート状態の復元
		ListView_SetSortedColumn(lv, column, ascending);
	}

	static void SaveSelection(IShellView* sv, CIDA** ppSelection, ITEMIDLIST** ppFocus)
	{
		CIDAPtr	selection = CIDACreate(sv, SVGIO_SELECTION);
		ILPtr	focus;
		int	index;
		ref<IFolderView> fv;
		if (fv || SUCCEEDED(sv->QueryInterface(&fv)))
			if SUCCEEDED(fv->GetFocusedItem(&index))
				fv->Item(index, &focus);
		*ppSelection = selection.detach();
		*ppFocus = focus.detach();
	}

	static void RestoreSelection(IShellView* sv, const CIDA* selection, const ITEMIDLIST* focus)
	{
		UINT svsi = SVSI_SELECT;
		// フォーカス優先
		if (focus)
			sv->SelectItem(focus, SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_DESELECTOTHERS);
		else
			svsi |= SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_DESELECTOTHERS;
		// フォーカスがなければ最初の選択項目をフォーカスする
		size_t count = CIDACount(selection);
		for (size_t i = 0; i < count; ++i)
		{
			if SUCCEEDED(sv->SelectItem(CIDAChild(selection, i), svsi))
				svsi &= ~(SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS);
		}
	}
}

void ListView::refresh()
{
	ref<IShellView>	sv;
	if FAILED(GetCurrentView(&sv))
		return;

	CIDAPtr	selection;
	ILPtr	focus;
	SaveSelection(sv, &selection, &focus);
	LockRedraw lock(m_hwnd);
	DoRefresh(sv, m_listview);
	RestoreSelection(sv, selection, focus);

	adjust_with_delay();
}

void ListView::recreate()
{
	if (!m_browser || !m_path)
		return;

	CIDAPtr	selection;
	ILPtr	focus;
	bool	hasFocus = HasFocus(m_hwnd);

	// 状態を保存する
	do
	{
		ref<IShellView>		sv;
		if SUCCEEDED(GetCurrentView(&sv))
		{
			sv->SaveViewState();
			sv->GetCurrentInfo(&m_settings);
			SaveSelection(sv, &selection, &focus);
		}
		m_settings.fFlags &= ~FWF_MASK;
		m_settings.fFlags |= FWF_SELECT[folderview_flag];
	} while (0);

	ExplorerDestroy(m_browser, m_cookie);
	if SUCCEEDED(ExplorerCreate(this, get_CenterArea(), m_settings, &m_browser, &m_cookie))
		m_browser->BrowseToIDList(m_path, SBSP_ABSOLUTE | SBSP_SAMEBROWSER);

	// 状態を復元保存する
	if (selection || focus)
	{
		ref<IShellView> sv;
		if SUCCEEDED(GetCurrentView(&sv))
			RestoreSelection(sv, selection, focus);
	}

	if (hasFocus && m_listview)
		m_listview.SetFocus();
}

void ListView::SetExtendedStyle(UINT style, bool value) throw()
{
	m_listview.SetExtendedStyle(style, value ? style : 0);
}

//==========================================================================================================================
// global configuration parameters

void set_show_hidden_files(bool value)
{
	if (show_hidden_files != value)
	{
		show_hidden_files = value;
		for (Window* w = Window::headobj(); w; w = w->nextobj())
			if (ListView* v = dynamic_cast<ListView*>(w))
				v->refresh();
	}
}

void set_folderview_flag(int value)
{
	if (folderview_flag != value && 0 <= value && value < lengthof(FWF_SELECT))
	{
		if (WINDOWS_VERSION < WINDOWS_VISTA && value == 3)
			throw sq::Error(L"Windows XP or eariler version does not support 3");
		// CHECK と CHECK3 を直接変更すると動作がおかしくなるので、いったんCHECK=OFFを経由する
		bool reset = ((folderview_flag == 3 && value == 2) ||
					  (folderview_flag == 2 && value == 3));
		folderview_flag = value;
		for (Window* w = Window::headobj(); w; w = w->nextobj())
		{
			if (ListView* v = dynamic_cast<ListView*>(w))
			{
				ref<IFolderView2> fv2;
				if SUCCEEDED(v->GetCurrentView(&fv2))
				{
					if (reset)
						fv2->SetCurrentFolderFlags(FWF_MASK, FWF_FULLROWSELECT);
					fv2->SetCurrentFolderFlags(FWF_MASK, FWF_SELECT[value]);
				}
				else
					v->recreate();
			}
		}
	}
}

void set_gridlines(bool value)
{
	if (gridlines != value)
	{
		gridlines = value;
		for (Window* w = Window::headobj(); w; w = w->nextobj())
			if (ListView* v = dynamic_cast<ListView*>(w))
				v->SetExtendedStyle(LVS_EX_GRIDLINES, value);
	}
}
