// listview.hpp
#pragma once

#include "commctrl.hpp"
#include "unknown.hpp"

// ICommDlgBrowser3 ÇÕñÇ…óßÇΩÇ»Ç¢ÅB

//=============================================================================
#ifdef ENABLE_LISTVIEW

#define FVM_HUGE	7	// 256	FVM_THUMBSTRIP
#define FVM_LARGE	5	//  96	FVM_THUMBNAIL
#define FVM_MEDIUM	1	//  48	FVM_ICON
#define FVM_SMALL	2	//  16	FVM_SMALLICON
//		FVM_LIST	3	//  16	FVM_LIST
//		FVM_DETAILS	4	//  16	FVM_DETAILS
//		FVM_TILE	6	//  48	FVM_TILE

#define ALL			(-1)
#define CHECKED		(-2)
#define CHECKED1	(-3)
#define CHECKED2	(-4)

class ListView :
	public Window,
	public Unknown<
		implements<
			IServiceProvider,
			IExplorerBrowserEvents,
			IExplorerPaneVisibility,
			IOleWindow,
			IShellBrowser,
			ICommDlgBrowser,
			ICommDlgBrowser2>,
		mixin<StaticLife> >
{
private:
	ref<IExplorerBrowser>	m_browser;
	DWORD					m_cookie;	// for m_browser->Advise()
	ILPtr					m_path;
	mutable FOLDERSETTINGS	m_settings;
	Hwnd					m_defview;
	ListViewH				m_listview;
	Hwnd					m_frame;
	bool					m_nav;
	bool					m_detail;
	bool					m_preview;

public:
	HRESULT	GetCurrentFolder(REFINTF pp) const throw();
	HRESULT	GetCurrentView(REFINTF pp) const throw();
	void	SetExtendedStyle(UINT style, bool value) throw();

	ref<IShellItem>	item(int index) const;
	ref<IEnumIDList> get_items() const;
	ref<IShellItem>	get_path() const;
	void			set_path(std::pair<PCWSTR, IShellItem*> value);
	ref<IShellItemArray> get_selection() const;
	SQInteger		set_selection(sq::VM v);
	SQInteger		get_columns(sq::VM v);
	SQInteger		set_columns(sq::VM v);

	bool	get_sorting() const;
	void	set_sorting(bool value);
	bool	get_nav() const;
	void	set_nav(bool value);
	bool	get_detail() const;
	void	set_detail(bool value);
	bool	get_preview() const;
	void	set_preview(bool value);
	int		get_mode() const;
	void	set_mode(int value);

	void	adjust_with_delay();
	void	adjust();
	void	bury();
	void	cut();
	void	copy();
	int		len() const throw();
	void	paste();
	void	newfile();
	void	newfolder();
	void	recreate();
	void	refresh();
	void	remove();
	void	rename();
	void	undo();

protected:
	LRESULT onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void	onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs);
	bool	onPress(const MSG& msg);
	bool	onGesture(const MSG& msg);
	HWND	onFocus() throw();

private:
	void	SetCurrentFolder(const ITEMIDLIST* path, bool copy) throw();
	void	SaveFolderSettings() const throw();
	void	RestoreFolderSettings() throw();
	void	DoPress(UINT vk, UINT mk = 0);
	HRESULT	onContextMenu(int x, int y) throw();
	void	onPreview(int index) throw();
	void	onPreviewUpdate() throw();
	void	set_selection_svsi(sq::VM v, SQInteger idx, UINT svsi);

	static LRESULT CALLBACK DefViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
	LRESULT	DefView_onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void	DefView_onBeginLabelEdit(NMLVDISPINFO* info);

	static LRESULT CALLBACK ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
	LRESULT	ListView_onMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK DirectUIHWNDProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
	LRESULT	DirectUIHWND_onMessage(UINT msg, WPARAM wParam, LPARAM lParam);

public:	// IServiceProvider
	IFACEMETHODIMP QueryService(REFGUID guidService, REFIID riid, void** pp);

public: // IExplorerBrowserEvents
	IFACEMETHODIMP OnNavigationPending(const ITEMIDLIST* folder);
	IFACEMETHODIMP OnViewCreated(IShellView* view);
	IFACEMETHODIMP OnNavigationComplete(const ITEMIDLIST* folder);
	IFACEMETHODIMP OnNavigationFailed(const ITEMIDLIST* folder);

public: // IExplorerPaneVisibility
	IFACEMETHODIMP GetPaneState(REFEXPLORERPANE ep, EXPLORERPANESTATE *peps);

public: // IOleWindow
	IFACEMETHODIMP GetWindow(HWND* phwnd);
	IFACEMETHODIMP ContextSensitiveHelp(BOOL fEnterMode)	{ return E_NOTIMPL; }

public: // ICommDlgBrowser
	IFACEMETHODIMP OnDefaultCommand(IShellView* view);
	IFACEMETHODIMP OnStateChange(IShellView* view, ULONG uChange);
	IFACEMETHODIMP IncludeObject(IShellView* view, const ITEMIDLIST* pidl);

public: // ICommDlgBrowser2
	IFACEMETHODIMP GetDefaultMenuText(IShellView* view, WCHAR* pszText, int cchMax);
	IFACEMETHODIMP Notify(IShellView* view, DWORD dwNotifyType);
	IFACEMETHODIMP GetViewFlags(DWORD* pdwFlags);

public: // ICommDlgBrowser3
//	IFACEMETHODIMP OnColumnClicked(IShellView *ppshv, int iColumn);
//	IFACEMETHODIMP GetCurrentFilter(LPWSTR pszFileSpec, int cchFileSpec);
//	IFACEMETHODIMP OnPreViewCreated(IShellView *ppshv);

public: // IShellBrowser
	IFACEMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)					{ return E_NOTIMPL; }
	IFACEMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuReserved, HWND hwndActiveObject)		{ return E_NOTIMPL; }
	IFACEMETHODIMP RemoveMenusSB(HMENU hmenuShared)														{ return E_NOTIMPL; }
	IFACEMETHODIMP SetStatusTextSB(PCWSTR text);
	IFACEMETHODIMP EnableModelessSB(BOOL fEnable)														{ return E_NOTIMPL; }
	IFACEMETHODIMP SetToolbarItems(TBBUTTON* buttons, UINT nButtons, UINT uFlags)						{ return E_NOTIMPL; }
	IFACEMETHODIMP TranslateAcceleratorSB(MSG* msg, WORD wID)											{ return E_NOTIMPL; }
	IFACEMETHODIMP OnViewWindowActive(IShellView* pShellView);
	IFACEMETHODIMP GetControlWindow(UINT id, HWND* pHwnd);
	IFACEMETHODIMP SendControlMsg(UINT id, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* lResult);
	IFACEMETHODIMP GetViewStateStream(DWORD stgm, IStream** pp)											{ return E_NOTIMPL; }
	IFACEMETHODIMP QueryActiveShellView(IShellView** pp);
	IFACEMETHODIMP BrowseObject(const ITEMIDLIST* pidl, UINT wFlags);
};

#endif
