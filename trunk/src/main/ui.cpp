// ui.cpp

#include "stdafx.h"
#include "sq.hpp"
#include "commctrl.hpp"
#include "listview.hpp"
#include "treeview.hpp"

namespace
{
	static HMENU	menu_handle;

	static void menu_fill(HMENU menu, UINT id)
	{
		if (sq::VM v = sq::vm)
		{
			SQInteger top = v.gettop();
			// top - 2 : callback
			// top - 1 : args
			// top     : menu object

			sq_push(v, top - 2);
			sq_push(v, top);

			if (id == 0)
				sq_push(v, top - 1);
			else
			{
				v.push(id);
				SQ_VERIFY(sq_rawget(v, top));
			}

			menu_handle = menu;
			sq::call(v, 2, SQFalse);
			menu_handle = null;
		}
	}

	// (object, flags, name)
	static SQInteger menu_append(sq::VM v)
	{
		if (!menu_handle)
			return sq::raise(v, E_UNEXPECTED, L"menu.append()");

		SQInteger	id = sq_getsize(v, 1);
		UINT		flags = v.get<UINT>(3);
		PCWSTR		name = v.get<PCWSTR>(4);
		MenuAppend(menu_handle, flags, (UINT)id, name);
		v.push(id);
		sq_push(v, 2);
		sq::rawset(v, 1);
		return 0;
	}

	// (fill, args, pt = null)
	static SQInteger menu(sq::VM v)
	{
		POINT pt;
		SQInteger top = v.gettop();

		if (top >= 4 && sq_gettype(v, 4) != OT_NULL)
			pt = v.get<POINT>(4);
		else
			pt = GetCursorPos();

		v.settop(3);

		v.newtable();
		v.def(L"append", menu_append);

		int id = MenuPopup(pt, menu_fill);
		if (id == 0)
			return 0;
		else
		{
			v.push(id);
			return SQ_SUCCEEDED(sq_rawget(v, 4)) ? 1 : SQ_ERROR;
		}
	}
}

static SQInteger modifiers(HSQUIRRELVM v) throw()
{
	sq::push(v, (GetModifiers() | GetButtons()) << 8);
	return 1;
}

//=============================================================================

#define def_event(name)		define_event(v, _SC(#name), L"get_" _SC(#name), L"set_"_SC(#name))
#define def_property(name)	do { def_method(get_##name); def_method(set_##name); } while (0)

// Window.get_onXXX(1:self) : (2:"onXXX")
static SQInteger get_slot(HSQUIRRELVM v)
{
	sq_pushstring(v, L"slots", -1);
	if SQ_SUCCEEDED(sq_get(v, 1))
	{
		sq_push(v, 2);
		if SQ_SUCCEEDED(sq_rawget(v, -2))
			return 1;
	}
	return 0;
}

// Window.set_onXXX(1:self, 2:sink) : (3:"onXXX")
static SQInteger set_slot(HSQUIRRELVM v)
{
	sq_pushstring(v, L"slots", -1);
	if SQ_SUCCEEDED(sq_get(v, 1))
	{
		bool init = (sq_gettype(v, -1) == OT_NULL);
		// 初回呼び出し時: テーブルを作成する
		if (init)
		{
			sq_pushstring(v, L"slots", -1);
			sq_newtable(v);
		}
		// table["onXXX"] = sink
		sq_push(v, 3);
		sq_push(v, 2);
		if SQ_FAILED(sq_rawset(v, -3))
			return SQ_ERROR;
		// 初回呼び出し時: this.slots = table
		if (init && SQ_FAILED(sq_rawset(v, 1)))
			return SQ_ERROR;
	}
	return 0;
}

static void define_event(sq::VM v, PCWSTR name, PCWSTR get, PCWSTR set)
{
	// get_onXXX
	v.push(get);
	v.push(name);
	sq_newclosure(v, get_slot, 1);
	sq_newslot(v, -3, SQFalse);

	// set_onXXX
	v.push(set);
	v.push(name);
	sq_newclosure(v, set_slot, 1);
	sq_newslot(v, -3, SQFalse);
}

SQInteger def_ui(sq::VM v)
{
	v.def(L"alert"		, Alert);
	v.def(L"beep"		, Sound);
	v.def(L"modifiers"	, modifiers);
	v.def(L"menu"		, menu);

#define CLASS	Window
	v.newclass<Window>();
	def_member(slots);
	def_property(dock);
	def_property(focus);
	def_property(font);
	def_property(name);
	def_property(width);
	def_property(height);
	def_property(location);
	def_property(size);
	def_property(visible);
	def_property(placement);
	def_method(get_children);
	def_method(get_parent);
	def_method(close);
	def_method(cut);
	def_method(copy);
	def_method(maximize);
	def_method(minimize);
	def_method(paste);
	def_method(press);
	def_method(move);
	def_method(resize);
	def_method(restore);
	def_method(run);
	def_event(onGesture);
	def_event(onPress);
	def_event(onClose);
	def_event(onDrop);
	v.newslot(-3);
#undef CLASS

#define CLASS	Dialog
	v.newclass<Dialog, Window>();
	def_method(run);
	v.newslot(-3);
#undef CLASS

#define CLASS	RenameDialog
	v.newclass<RenameDialog, Dialog>();
	def_method(run);
	def_method(set_items);
	def_method(set_names);
	v.newslot(-3);
#undef CLASS

#ifdef ENABLE_TEXTVIEW
#define CLASS	TextView
	v.newclass<TextView, Window>();
	def_property(readonly);
	def_property(name);
	def_property(selection);
	def_property(text);
	def_method(append);
	def_method(clear);
	def_method(undo);
	v.newslot(-3);
#undef CLASS
#endif

#ifdef ENABLE_TOOLBAR
#define CLASS	ToolBar
	v.newclass<ToolBar, Window>();
	def_property(icons);
	def_property(items);
	def_method(menupos);
	def_method(len);
	def_event(onDropDown);
	def_event(onExecute);
	v.newslot(-3);
#undef CLASS
#endif

#ifdef ENABLE_STATUSBAR
#define CLASS	StatusBar
	v.newclass<StatusBar, Window>();
	def_property(name);
	def_property(text);
	v.newslot(-3);
#undef CLASS
#endif

#ifdef ENABLE_REBAR
#define CLASS	ReBar
	v.newclass<ReBar, Window>();
	def_property(locked);
	v.newslot(-3);
#undef CLASS
#endif

#ifdef ENABLE_LISTVIEW
#define CLASS	ListView
	v.newclass<ListView, Window>();
	def_property(columns);
	def_property(detail);
	def_property(mode);
	def_property(nav);
	def_property(path);
	def_property(preview);
	def_property(selection);
	def_property(sorting);
	def_method(adjust);
	def_method(bury);
	def_method(get_items);
	def_method(item);
	def_method(len);
	def_method(newfile);
	def_method(newfolder);
	def_method(refresh);
	def_method(remove);
	def_method(rename);
	def_method(undo);
	def_event(onExecute);
	def_event(onNavigate);
	v.newslot(-3);
#undef CLASS
#endif

#ifdef ENABLE_TREEVIEW
#define CLASS	TreeView
	v.newclass<TreeView, Window>();
	def_method(append);
	def_method(collapse);
	def_method(only);
	def_method(toggle);
	def_property(selection);
	def_event(onExecute);
	v.newslot(-3);
#undef CLASS
#endif

	return 0;
}
