// config.cpp

#include "stdafx.h"
#include "sq.hpp"
#include "win32.hpp"

//================================================================================

extern SQInteger get_hotkeys(sq::VM v);
extern SQInteger set_hotkeys(sq::VM v);

extern void	set_full_row_select(bool value);
extern void	set_gridlines(bool value);
extern void	set_show_hidden_files(bool value);
extern void	set_folderview_flag(int value);

extern bool		navigation_pane;
extern bool		detail_pane;
extern bool		preview_pane;
extern bool		gridlines;
extern bool		loop_cursor;
extern bool		show_hidden_files;
extern int		floating_preview;
extern int		folderview_flag;
extern int		folderview_mode;
extern WCHAR	navigation_sound[MAX_PATH];

namespace
{
	template < bool* Var >	SQInteger get_bool(sq::VM v)	{ v.push(*Var); return 1; }
	template < bool* Var >	SQInteger set_bool(sq::VM v)	{ *Var = v.get<bool>(2); return 0; }
	template < int*  Var >	SQInteger get_int(sq::VM v)		{ v.push(*Var); return 1; }
	template < int*  Var >	SQInteger set_int(sq::VM v)		{ *Var = v.get<int>(2); return 0; }
	template < PWSTR Var >	SQInteger get_str(sq::VM v)		{ v.push(Var); return 1; }
	template < PWSTR Var >	SQInteger set_str(sq::VM v)		{ wcscpy_s(Var, MAX_PATH, v.get<PCWSTR>(2)); return 0; }
}

//================================================================================

#define def_bool(name) \
	do  { \
		__if_exists    (get_##name) { v.def(L"get_"_SC(#name), get_##name ); } \
		__if_not_exists(get_##name) { v.def(L"get_"_SC(#name), get_bool<&name>); } \
		__if_exists    (set_##name) { v.def(L"set_"_SC(#name), set_##name ); } \
		__if_not_exists(set_##name) { v.def(L"set_"_SC(#name), set_bool<&name>); } \
	} while (0)

#define def_int(name) \
	do  { \
		__if_exists    (get_##name) { v.def(L"get_"_SC(#name), get_##name ); } \
		__if_not_exists(get_##name) { v.def(L"get_"_SC(#name), get_int<&name>); } \
		__if_exists    (set_##name) { v.def(L"set_"_SC(#name), set_##name ); } \
		__if_not_exists(set_##name) { v.def(L"set_"_SC(#name), set_int<&name>); } \
	} while (0)

#define def_str(name) \
	do  { \
		__if_exists    (get_##name) { v.def(L"get_"_SC(#name), get_##name ); } \
		__if_not_exists(get_##name) { v.def(L"get_"_SC(#name), get_str<name>); } \
		__if_exists    (set_##name) { v.def(L"set_"_SC(#name), set_##name ); } \
		__if_not_exists(set_##name) { v.def(L"set_"_SC(#name), set_str<name>); } \
	} while (0)

SQInteger def_config(sq::VM v)
{
	// 環境依存の設定パラメータを初期化する。
	folderview_flag = (WINDOWS_VERSION < WINDOWS_VISTA ? 0 : 1);

	// object から派生する無名クラスを作成する。
	sq_pushroottable(v);
	sq::push(v, L"object");
	sq_rawget(v, -2);
	sq_newclass(v, SQTrue);

	// 無名クラスにグローバル変数ハンドラを登録する。
	def_bool(navigation_pane);
	def_bool(detail_pane);
	def_bool(preview_pane);
	def_bool(gridlines);
	def_bool(loop_cursor);
	def_bool(show_hidden_files);

	def_int(floating_preview);
	def_int(folderview_flag);
	def_int(folderview_mode);

	def_str(navigation_sound);

	v.def(L"get_keymap", get_hotkeys);
	v.def(L"set_keymap", set_hotkeys);

	// グローバル変数 config を作成する。
	v.push(L"config");
	SQ_VERIFY( sq_createinstance(v, -2) );
	v.newslot(-5);

	// 無名クラスは登録する必要がない。root と共に取り除く。
	v.pop(2);

	return 1;
}
