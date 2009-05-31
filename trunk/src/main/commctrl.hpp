// commctrl.hpp
#pragma once

#include "ref.hpp"
#include "win32_commctrl.hpp"
#include "window.hpp"

#define ENABLE_TEXTVIEW
#define ENABLE_LISTVIEW
#define ENABLE_TREEVIEW
#define ENABLE_TOOLBAR
//#define ENABLE_STATUSBAR
#define ENABLE_REBAR
#define ENABLE_RENAMEDLG

//=============================================================================

class Dialog : public Window
{
private:
	Hwnd		m_owner;
	ToolTipH	m_tip;

public:
	void	run();
	void	setTip(HWND hwnd, UINT message);

protected:
	virtual void	onClose(int id);
	LRESULT	onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void	onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs);
	bool	onPress(const MSG& msg) throw();
	bool	onGesture(const MSG& msg);
};

//=============================================================================
#ifdef ENABLE_TEXTVIEW

class TextView : public EditT<Window>
{
private:
	mutable bool	m_internalText;

public:
	TextView();

	bool	get_readonly() const throw();
	void	set_readonly(bool value) throw();
	POINT	get_selection();
	void	set_selection(POINT value);
	string	get_name() const throw();
	void	set_name(PCWSTR value) throw();
	string	get_text() const throw();
	void	set_text(PCWSTR value) throw();

	void	append(PCWSTR text);
	void	clear() throw();
	void	undo() throw();

protected:
	LRESULT	onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void	onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs);
	bool	onGesture(const MSG& msg);
};

#endif
//=============================================================================
#ifdef ENABLE_TOOLBAR

class ToolBar : public ToolBarT<Window>
{
private:
	HSQOBJECT		m_items;
	ref<IImageList>	m_icons;
	bool			m_inDropDown;

public:
	void		constructor(sq::VM v);
	IImageList*	get_icons() const;
	void		set_icons(IImageList* value);
	SQInteger	get_items(sq::VM v);
	SQInteger	set_items(sq::VM v);
	int			len() const throw()	{ return GetItemCount() - 1; }
	POINT		menupos(int index) const;

protected:
	LRESULT	onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT	onNotify(Window* from, NMHDR& nm);
	void	onUpdate(RECT& bounds) throw();
	void	onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs);
	bool	onGesture(const MSG& msg);
	Window*	onDragOver(DWORD keys, POINT ptScreen, DWORD* effect);
	void	onDragDrop(IDataObject* data, DWORD keys, POINT ptScreen, DWORD* effect);
	void	onDragLeave();

private:
	bool	onMouseDown(UINT msg, WPARAM wParam, LPARAM lParam);
	void	onExecute(int index, UINT vk, UINT mk);
	bool	onDropDown(int index);
};

#endif
//=============================================================================
#ifdef ENABLE_STATUSBAR

class StatusBar : public StatusBarT<Window>
{
private:
	mutable bool	m_internalText;

public:
	StatusBar();

	string	get_name() const throw();
	void	set_name(PCWSTR value) throw();
	string	get_text() const throw();
	void	set_text(PCWSTR value) throw();

protected:
	LRESULT	onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void	onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs);
};

#endif
//=============================================================================
#ifdef ENABLE_REBAR

class ReBar : public ReBarT<Window>
{
protected:
	LRESULT	onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT	onNotify(Window* from, NMHDR& nm);
	void	onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs);
	void	onUpdate(RECT& bounds) throw();

public:
	bool		get_locked() const throw();
	void		set_locked(bool value) throw();
	SQInteger	get_placement(sq::VM v);
	SQInteger	set_placement(sq::VM v);

private:
	void	onChildChange(bool create, Hwnd child) throw();
};

#endif
//=============================================================================
#ifdef ENABLE_RENAMEDLG

class RenameDialog : public Dialog
{
private:
	ListViewH	m_listview;
	ButtonH		m_ok;
	ButtonH		m_cancel;
	EditH		m_format;
	bool		m_renamed;
	ref<IFileOperation>	m_op;
	std::vector< ref<IShellItem> >	m_items;

public:
	void		run();
	void		exclude(int index);
	void		revert(int index);
	SQInteger	set_items(sq::VM v);
	SQInteger	set_names(sq::VM v);

protected:
	void	onClose(int id);
	LRESULT	onMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	bool	onPress(const MSG& msg) throw();

private:
	void	onDialogCreate();
	void	onDialogResize(int w, int h);
};

#endif
//==========================================================================================================================

extern void SubclassSingleLineEdit(HSQUIRRELVM v, HWND hwnd);

extern PCWSTR	item_name(HSQUIRRELVM v, SQInteger idx);
extern int		item_icon(HSQUIRRELVM v, SQInteger idx);
extern int		item_check(HSQUIRRELVM v, SQInteger idx);
