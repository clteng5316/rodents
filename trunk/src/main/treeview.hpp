// treeview.hpp
#pragma once

#include "commctrl.hpp"
#include "unknown.hpp"

//=============================================================================
#ifdef ENABLE_TREEVIEW

class TreeView :
	public Window,
	public Unknown<
		implements<
			IServiceProvider,
			INameSpaceTreeControlEvents>,
		mixin<StaticLife> >
{
private:
    ref<INameSpaceTreeControl>	m_control;
	DWORD						m_cookie;
	TreeViewH					m_treeview;
	bool						m_inExecute;

public:
	ref<IShellItem>	get_selection() const;
	void	set_selection(IShellItem* value);
	void	append(IShellItem* item);
	void	collapse(IShellItem* value);
	void	only(IShellItem* value);
	void	toggle(IShellItem* value);

protected:
	LRESULT onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void	onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs);
//	bool	onPress(const MSG& msg);
	bool	onGesture(const MSG& msg);
	HWND	onFocus() throw();

public:	// IServiceProvider
	IFACEMETHODIMP QueryService(REFGUID guidService, REFIID riid, void** pp);

public: // INameSpaceTreeControlEvents
    IFACEMETHODIMP OnItemClick(IShellItem* item, NSTCEHITTEST hit, NSTCECLICKTYPE click);
    IFACEMETHODIMP OnPropertyItemCommit(IShellItem* item);
    IFACEMETHODIMP OnItemStateChanging(IShellItem* item, NSTCITEMSTATE mask, NSTCITEMSTATE state);
    IFACEMETHODIMP OnItemStateChanged(IShellItem* item, NSTCITEMSTATE mask, NSTCITEMSTATE state);
    IFACEMETHODIMP OnSelectionChanged(IShellItemArray* items);
    IFACEMETHODIMP OnKeyboardInput(UINT msg, WPARAM wParam, LPARAM lParam);
    IFACEMETHODIMP OnBeforeExpand(IShellItem* item);
    IFACEMETHODIMP OnAfterExpand(IShellItem* item);
    IFACEMETHODIMP OnBeginLabelEdit(IShellItem* item);
    IFACEMETHODIMP OnEndLabelEdit(IShellItem* item);
    IFACEMETHODIMP OnGetToolTip(IShellItem* item, LPWSTR pszTip, int cchTip);
    IFACEMETHODIMP OnBeforeItemDelete(IShellItem* item);
    IFACEMETHODIMP OnItemAdded(IShellItem* item, BOOL root);
    IFACEMETHODIMP OnItemDeleted(IShellItem* item, BOOL root);
    IFACEMETHODIMP OnBeforeContextMenu(IShellItem* item, REFIID iid, void** pp);
    IFACEMETHODIMP OnAfterContextMenu(IShellItem* item, IContextMenu* menu, REFIID iid, void** pp);
    IFACEMETHODIMP OnBeforeStateImageChange(IShellItem* item);
    IFACEMETHODIMP OnGetDefaultIconIndex(IShellItem* item, int* defaultIcon, int* openIcon);
};

#endif
