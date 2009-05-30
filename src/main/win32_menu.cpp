// win32_menu.cpp
#include "stdafx.h"
#include "math.hpp"
#include "ref.hpp"
#include "string.hpp"
#include "win32.hpp"
#include "resource.h"

//==========================================================================================================================

namespace
{
	const UINT	RESERVED_NUM	= 0x0FFF;
	const UINT	ID_SHELL_FIRST	= 0x7000;
	const UINT	ID_SHELL_LAST	= ID_SHELL_FIRST + RESERVED_NUM - 1;
	const UINT	ID_DEFAULT		= 0xFFFF;

	HRESULT GetCommandString(IContextMenu* menu, UINT uID, WCHAR name[MAX_PATH], UINT gcsW, UINT gcsA) throw()
	{
		HRESULT hr;
		if SUCCEEDED(menu->GetCommandString(uID, gcsW, null, (LPSTR)name, MAX_PATH))
			return S_OK;
		CHAR nameA[MAX_PATH];
		if FAILED(hr = menu->GetCommandString(uID, gcsA, null, nameA, MAX_PATH))
			return hr;
		MultiByteToWideChar(CP_ACP, 0, nameA, -1, name, MAX_PATH);
		return S_OK;
	}

	bool IsRenameCommand(IContextMenu* menu, UINT uID) throw()
	{
		WCHAR name[MAX_PATH];
		return SUCCEEDED(GetCommandString(menu, uID, name, GCS_VERBW, GCS_VERBA))
			&& _wcsicmp(name, L"rename") == 0;
	}

	UINT GetMenuItemType(HMENU handle, INT pos) throw()
	{
		MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_FTYPE };
		if (GetMenuItemInfo(handle, pos, TRUE, &info))
			return info.fType;
		else
			return 0;
	}

	HRESULT AppendItems(IContextMenu* menu, HMENU handle, UINT first, UINT last, RECT* rc = null)
	{
		HRESULT hr;
		if (!menu)
			return E_POINTER;

		// 改行を追加する。
		int length = GetMenuItemCount(handle);
		if (length >= 0)
		{
			MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_FTYPE };
			if (GetMenuItemInfo(handle, 0, TRUE, &info))
			{
				info.fType |= MFT_MENUBREAK;
				SetMenuItemInfo(handle, 0, TRUE, &info);
			}
			else
			{
				length = 0; // unknown error
			}
		}

		// CMF_CANRENAME : drag-and-drop の場合は使わないべき
		// CMF_EXTENDEDVERBS : SHIFT+RClick の場合に拡張コマンドを表示すべき
		if FAILED(hr = menu->QueryContextMenu(handle, 0, first, last, CMF_CANRENAME | CMF_EXPLORE))
			return hr;

		// セパレータの直前に改行が挿入されるため、それを取り除く。
		if (length > 0)
		{
			int menubreak = GetMenuItemCount(handle) - length;
			if (GetMenuItemType(handle, menubreak) & MFT_MENUBREAK)
				if (GetMenuItemType(handle, menubreak-1) & MFT_SEPARATOR)
					DeleteMenu(handle, menubreak-1, MF_BYPOSITION);
		}

		return S_OK;
	}

	static inline bool IsMenuMessage(UINT msg)
	{
		switch (msg)
		{
		case WM_MENUDRAG:
		case WM_MENUGETOBJECT:
		case WM_MENUSELECT:
		case WM_INITMENUPOPUP:
		case WM_UNINITMENUPOPUP:
		case WM_MENUCHAR:
		case WM_MENURBUTTONUP:
		case WM_MEASUREITEM:
		case WM_DRAWITEM:
			return true;
		default:
			return false;
		}
	}

	LRESULT CALLBACK ContextMenuProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data) throw()
	{
		if (IsMenuMessage(msg))
		{
			LRESULT lResult;
			IContextMenu* menu = (IContextMenu*)data;

			ref<IContextMenu3> menu3;
			ref<IContextMenu2> menu2;
			if SUCCEEDED(menu->QueryInterface(&menu3))
			{
				if (menu3->HandleMenuMsg2(msg, wParam, lParam, &lResult) == S_OK)
					return lResult;
			}
			else if SUCCEEDED(menu->QueryInterface(&menu2))
			{
				if (menu3->HandleMenuMsg(msg, wParam, lParam) == S_OK)
					return 0;
			}
		}
		return ::DefSubclassProc(hwnd, msg, wParam, lParam);
	}
}

HRESULT MenuPopup(POINT pt, IContextMenu* menu, PCWSTR defaultCommand) throw()
{
	HRESULT	hr;

	MenuH handle = ::CreatePopupMenu();
	if FAILED(hr = AppendItems(menu, handle, ID_SHELL_FIRST, ID_SHELL_LAST))
		return hr;

	if (!str::empty(defaultCommand))
	{
		MENUITEMINFO item = { sizeof(MENUITEMINFO) };

		// 2番目はセパレータ
		item.fMask = MIIM_FTYPE;
		item.fType = MFT_SEPARATOR;
		InsertMenuItem(handle, 0, true, &item);

		// 1番目はデフォルト項目
		item.fMask = MIIM_STRING | MIIM_ID;
		item.wID = ID_DEFAULT;
		item.dwTypeData = const_cast<PWSTR>(defaultCommand);
		InsertMenuItem(handle, 0, true, &item);

		// デフォルト指定は MFS_DEFAULT ではなく、SetMenuDefauItem で行う。
		SetMenuDefaultItem(handle, 0, true);
	}

	HWND hwnd = GetWindow();

	if (hwnd)
		::SetWindowSubclass(hwnd, ContextMenuProc, 0, (DWORD_PTR)(IContextMenu*)menu);
	INT id = ::TrackPopupMenuEx(handle, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_RECURSE, pt.x, pt.y, hwnd, null);
	if (hwnd)
		::RemoveWindowSubclass(hwnd, ContextMenuProc, 0);

	if (id == 0)
	{	// キャンセル
		return S_FALSE;
	}

	// メニューを完全に閉じた後に実行する。
	EndMenu();
	PumpMessage();

	if (id == ID_DEFAULT)
	{
		return S_DEFAULT;
	}
	else if (ID_SHELL_FIRST <= id && id <= ID_SHELL_LAST)
	{	// シェルメニュー
		UINT offset = id - ID_SHELL_FIRST;
		if (IsRenameCommand(menu, offset))
		{
			return S_RENAME;
		}
		else
		{
			CMINVOKECOMMANDINFO info = { sizeof(CMINVOKECOMMANDINFO) };
			info.hwnd   = hwnd;
			info.lpVerb = MAKEINTRESOURCEA(offset);
			info.nShow  = SW_SHOWNORMAL;
			menu->InvokeCommand(&info);
			return S_OK;	// InvokeCommand の成功失敗によらず常に S_OK を返す。
		}
	}
	else
		return S_FALSE; // unknown
}

HRESULT MenuPopup(POINT pt, IShellView* view, ICommDlgBrowser2* browser) throw()
{
	HRESULT hr;
	ref<IContextMenu> menu;
	if FAILED(hr = view->GetItemObject(SVGIO_SELECTION, IID_PPV_ARGS(&menu)))
		return S_FALSE;

	WCHAR text[MAX_PATH] = L"";
	hr = browser->GetDefaultMenuText(view, text, MAX_PATH);

	return MenuPopup(pt, menu, SUCCEEDED(hr) ? text : null);
}

//==========================================================================================================================

namespace
{
	const DWORD MENU_FILLED	= 0xDEADBEEF;	// 初期化済みフラグ。ContextHelpId に設定される

	static HHOOK				MenuHook;
	static std::vector<HMENU>	MenuStack;	//
	static int					ResultID;	// 標準でない方法で実行する場合のコマンドID
	static MenuFill				Callback;

	// スタックを辿り、menu の親を探す。
	static HMENU GetMenuParent(HMENU menu)
	{
		ssize_t	index = 0;
		for (index = MenuStack.size() - 1; index >= 0; --index)
		{
			if (MenuStack[index] == menu)
				break;
		}
		if (index <= 0)
			return null;
		for (--index; index >= 0; --index)
		{
			HMENU	parent = MenuStack[index];
			int	count = GetMenuItemCount(parent);
			for (int i = 0; i < count; ++i)
			{
				if (GetSubMenu(parent, i) == menu)
					return parent;
			}
		}
		return null;
	}

	// メニュースタックを辿って指定した位置にあるアイテムを探す。
	static bool MenuItemFromPoint(POINT pt, HMENU* o_menu, int* o_index)
	{
		if (!IsMenuWindow(WindowFromPoint(pt)))
			return false;

		for (ssize_t i = MenuStack.size() - 1; i >= 0; --i)
		{
			HMENU menu = MenuStack[i];
			int index = ::MenuItemFromPoint(null, menu, pt);
			if (index >= 0)
			{
				*o_menu = menu;
				*o_index = index;
				return true;
			}
		}
		return false;
	}

	// 実行する場合は S_OK, しない場合は S_FALSE, HILITE でない場合は E_FAIL
	static HRESULT MenuExecute(HMENU menu, int pos, bool hiliteOnly)
	{
		MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_ID | MIIM_SUBMENU | MIIM_STATE };
		if (!GetMenuItemInfo(menu, pos, true, &info))
			return S_FALSE;

		// 選択されている項目のみ
		if (hiliteOnly && (info.fState & MFS_HILITE) == 0)
			return S_FALSE;

		// 選択項目が無効化されている場合、残りの判定はすべて失敗するため。
		if (info.fState & MFS_DISABLED)
			return E_FAIL;
		// ID が無い場合は実行できない。
		if (info.wID == 0)
			return E_FAIL;

		// この項目が選択されたと見なす。
		// MenuExecute() すると 0 が返ってしまうので、グローバル変数に結果を保存する。
		// XXX: 再起呼び出しに備えスタック別にすべき？
		ResultID = info.wID;
		::EndMenu();
		return S_OK;
	}

	static LRESULT CALLBACK OnMouseHook(int code, WPARAM wParam, LPARAM lParam)
	{
		if (code == HC_ACTION)
		{
			MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
			HMENU	menu;
			int		pos;

			switch (wParam)
			{
			case WM_RBUTTONUP:
				// サブメニューが開いていると、その親メニューでは WM_MENURBUTTONUP が
				// 送られなくなる。そのため、自前でイベントを発生させる。
				if (MenuItemFromPoint(info->pt, &menu, &pos))
					PostMessage(GetWindow(), WM_MENURBUTTONUP, pos, (LPARAM)menu);
				break;
			case WM_LBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
//			case WM_MBUTTONUP:	// TODO: UPにしたい
				// ポップアップ項目は左ダブルクリックまたは中クリックで起動できる。
				if (MenuItemFromPoint(info->pt, &menu, &pos))
					MenuExecute(menu, pos, false);
				break;
			}
		}
		return ::CallNextHookEx(MenuHook, code, wParam, lParam);
	}

	static void OnInitMenuPopup(HMENU menu, int index) throw()
	{
		UINT id = 0;

		MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_ID | MIIM_SUBMENU };
		for (ssize_t i = MenuStack.size() - 1; i >= 0; --i)
		{
			if (GetMenuItemInfo(MenuStack[i], index, true, &info) &&
				info.hSubMenu == menu)
			{
				id = info.wID;
				break;
			}
		}

		MenuStack.push_back(menu);

		DWORD magic = GetMenuContextHelpId(menu);
		if (magic == MENU_FILLED)
			return;	// 既に初期化が完了している。
		SetMenuContextHelpId(menu, MENU_FILLED);

		if (Callback)
			Callback(menu, id);

		if (GetMenuItemCount(menu) == 0)
		{	// 空の場合は "(empty)" を挿入する。
			MenuAppend(menu, 0, 0, _S(STR_EMPTY));
		}
	}

	static void OnUninitMenuPopup(HMENU menu) throw()
	{
		for (ssize_t i = MenuStack.size() - 1; i >= 0; --i)
		{
			if (menu == MenuStack[i])
			{
				MenuStack.erase(MenuStack.begin() + i);
				break;
			}
		}
	}

	static bool OnMenuChar(UINT type, UINT ch, HMENU menu, LRESULT& lResult)
	{
		// スペースが押された場合、そのとき選択されているアイテムを実行する。
		if (type == MF_POPUP && ch == ' ')
		{
			int count = ::GetMenuItemCount(menu);
			for (int i = 0; i < count; ++i)
			{
				HRESULT hr = MenuExecute(menu, i, true);
				if (hr == S_OK)
				{
					lResult = MAKELRESULT(i, MNC_SELECT);
					return true;
				}
				else if FAILED(hr)
					return false;
			}
			// 親の中から探す
			if (HMENU parent = GetMenuParent(menu))
				return OnMenuChar(type, ch, parent, lResult);
		}
		return false;
	}

	static LRESULT CALLBACK OnMenuMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) throw()
	{
		switch (msg)
		{
	///	MNS_DRAGDROP
	//	case WM_MENUDRAG:
	//	case WM_MENUGETOBJECT:
		case WM_INITMENUPOPUP:
			OnInitMenuPopup((HMENU)wParam, LOWORD(lParam));
			break;
		case WM_UNINITMENUPOPUP:
			OnUninitMenuPopup((HMENU)wParam);
			break;
		case WM_MENUCHAR:
			LRESULT lResult;
			if (OnMenuChar(HIWORD(wParam), LOWORD(wParam), (HMENU)lParam, lResult))
				return lResult;
			break;
		}
		return ::DefSubclassProc(hwnd, msg, wParam, lParam);
	}
}

HMENU MenuAppend(HMENU menu, UINT flags, UINT id, PCWSTR name)
{
	MENUITEMINFO item = { sizeof(MENUITEMINFO), MIIM_FTYPE };
	if (flags & MF_SEPARATOR)
	{
		item.fType |= MFT_SEPARATOR;
	}
	else
	{
		item.fMask |= MIIM_STRING | MIIM_ID;
		item.dwTypeData = const_cast<PWSTR>(name);
		item.wID = id;

		// WM_DRAWITEM で描画するために、ダミーのIDを割り当てる。
		if (flags & MF_POPUP)
		{
			item.fMask |= MIIM_SUBMENU;
			item.hSubMenu = ::CreatePopupMenu();
		}
		else if (id > 0)
		{
		}
		else
		{
			flags |= MF_DISABLED;
		}

		// State
		if (flags & MF_DISABLED)
		{
			item.fMask |= MIIM_STATE;
			item.fState = MFS_GRAYED | MFS_DISABLED;
		}
		if (flags & MF_CHECKED)
		{
			item.fMask |= MIIM_STATE;
			item.fState = MFS_CHECKED;
		}
	}

	::InsertMenuItem(menu, (UINT)-1, true, &item);

	if (!item.hSubMenu)
		return null;

	return item.hSubMenu;
}

int MenuPopup(POINT ptScreen, MenuFill fill) throw()
{
	if (MenuHook != 0)
		return 0;	// すでにメニューが実行中。or EndMenu() すべき？

	HWND hwndOwner = GetWindow();
	if (!hwndOwner)
		return 0;	// イベントハンドリングのため、ウィンドウは必須。

	MenuHook = ::SetWindowsHookEx(WH_MOUSE, OnMouseHook, ::GetModuleHandle(null), GetCurrentThreadId());
	::SetWindowSubclass(hwndOwner, OnMenuMessage, 0, 0);
	ResultID = 0;
	Callback = fill;

	HMENU menu = ::CreatePopupMenu();
	int r = ::TrackPopupMenuEx(menu, TPM_RETURNCMD, ptScreen.x, ptScreen.y, hwndOwner, null);
	if (r == 0 && ResultID != 0)
		r = ResultID;
	::DestroyMenu(menu);
	ResultID = 0;
	Callback = null;
	MenuStack.clear();
	::RemoveWindowSubclass(hwndOwner, OnMenuMessage, 0);
	::UnhookWindowsHookEx(MenuHook);
	MenuHook = null;

	return r;
}
