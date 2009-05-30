// window.hpp
#pragma once

#include "hwnd.hpp"
class Thread;

enum DIRECTION
{
	NONE,
	NORTH,
	SOUTH,
	WEST,
	EAST,
	CENTER,
};

class Window : public object, public Hwnd
{
private:
	HSQOBJECT	m_self;
	Window*		m_nextobj;
	Hwnd		m_focus;
	BYTE		m_dock;
	bool		m_pressed;
	bool		m_updated;

protected:
	virtual ~Window();

public:
	Window();

	static Window* from(HWND hwnd);
	static Window* headobj();
	Window* nextobj() const;

	void	constructor(sq::VM v);
	void	finalize();

	HSQOBJECT	get_self() const throw()	{ return m_self; }

	string	get_name() const throw();
	void	set_name(PCWSTR value) throw();
	BYTE	get_dock() const;
	void	set_dock(BYTE value) throw();
	HWND	get_focus() const throw()	{ return m_focus; }
	void	set_focus(HWND hwnd) throw();
	string	get_font() throw();
	void	set_font(PCWSTR value) throw();

	SQInteger	get_children(sq::VM v);
	SQInteger	get_placement(sq::VM v);
	SQInteger	set_placement(sq::VM v);

	SIZE	get_DefaultSize() const throw();
	RECT	get_CenterArea() const throw();

	void	update() throw();
	void	press(UINT key) throw();
	void	press_with_mods(UINT vk, UINT mk) throw();
	void	run() throw();

	typedef void (*Handler)(sq::VM, LPARAM);
	bool	onEvent(PCWSTR name, Handler handler = 0, LPARAM arg = 0);

	template < typename T >
	bool onEvent(PCWSTR name, void (*handler)(sq::VM, T), T arg)
	{
		return onEvent(name, (Handler)handler, (LPARAM)arg);
	}

	template < typename T >
	bool onEvent(PCWSTR name, T arg)
	{
		STATIC_ASSERT(sizeof(T) == sizeof(LPARAM));
		return onEvent(name, (Handler)_push<T>, (LPARAM)arg);
	}

private:
	template < typename T >
	static void _push(sq::VM v, T value) { v.push(value); }

protected:
	virtual LRESULT onMessage(UINT msg, WPARAM wParam, LPARAM lParam) throw();
	virtual LRESULT onNotify(Window* from, NMHDR& nm) throw();
	virtual void	onCreate(WNDCLASSEX& wc, CREATESTRUCT& cs) throw();
	virtual bool	onPress(const MSG& msg) throw();
	virtual bool	onGesture(const MSG& msg) throw();
	virtual void	onUpdate(RECT& bounds) throw();
	virtual HWND	onFocus() throw();
	virtual Window*	onDragOver(DWORD keys, POINT ptScreen, DWORD* effect);
	virtual void	onDragDrop(IDataObject* data, DWORD keys, POINT ptScreen, DWORD* effect);
	virtual void	onDragLeave();

private:
	void	onContextMenu(int x, int y) throw();
	void	doUpdate() throw();
	static bool	handlePress(const MSG& msg);
	static bool	handleGesture(const MSG& msg);
	static LRESULT CALLBACK onMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
	friend class Thread;
};
