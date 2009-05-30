﻿form <- null
list <- null
tree <- null

//=============================================================================
// ListView commands

function go_up()		{ this.set_path("..") }
function go_back()		{ this.set_path("<") }
function go_forward()	{ this.set_path(">") }

local function copy_selection(attr)
{
	local text
	local items = this.selection
	foreach (i in items)
		text = concat(text, i[attr])
	if (text)
		os.clipboard_set_text(text)
}

function copy_name()	{ copy_selection("name") }
function copy_path()	{ copy_selection("path") }

function execute_one(verb)
{
	local items = this.selection
	if (items)
	{
		local item = items[0].linked
		if (item.browsable(false))
			item = item.children[0]
		item.execute(verb)
	}
}

function execute_dir(verb)
{
	this.path.execute(verb)
}

function add_to_bookmark()
{
	local view = (this instanceof ListView) ? this : form.focus
	local item = view.path
	item.symlink("Links/" + item.name)
	// TODO: Update link bar
}

function copy_to_other()
{
	alert("copy_to_other")
}

function move_to_other()
{
	local columns = this.columns
	foreach (c in columns)
		print("%s / %s / %d\n", c.key, c.name, c.width)
}

function paste_into()
{
	local items = this.selection
	if (!items || items.len() != 1 || !os.paste(items[0].linked.path))
		beep()
}

function rename_dialog()
{
	local items = this.selection
	if (items == null || items.len() == 0)
		return
	local sz = ::form.size
	local dlg = RenameDialog(::form)
	dlg.name = "名前変更"
	dlg.keymap = RenameDialog_keymap
	dlg.size = [sz[0] * 0.8, sz[1] * 0.6]
	dlg.items = items
	dlg.run()
}

function paste_name()
{
	local items = this.selection
	if (items == null)
		return
	local len = items.len()
	if (len <= 0)
		return
	local text = os.clipboard_get_text()
	if (!text)
		return
	local names = split(text, "\n")
	local sz = ::form.size
	local dlg = RenameDialog(::form)
	dlg.name = "名前変更"
	dlg.size = [sz[0] * 0.8, sz[1] * 0.6]
	dlg.items = items
	dlg.names = (len == 1 || names.len() > 1 ? names : text)
	dlg.run()
}

//=============================================================================
// TreeView commands

function FolderTree_toggle(path)
{
	if (::tree && ::tree.visible)
		::tree.toggle(this.path)
}

function FolderTree_only(path)
{
	if (::tree && ::tree.visible)
		::tree.only(path)
}

//=============================================================================
// Editor commands

// ファイル名の先頭 or 拡張子の先頭
function editor_home()
{
	local text = this.text
	local dot = text.rfind(".")
	if (dot == null || dot == 0)
	{
		this.press(VK_HOME)
		return
	}
	local cur = this.selection
	if (cur[0] == 0 && cur[1] == 0)
		this.selection = [dot + 1, dot + 1]
	else
		this.press(VK_HOME)
}

// ファイル名の末尾 or 拡張子の末尾
function editor_end()
{
	local text = this.text
	local dot = text.rfind(".")
	if (dot == null || dot == 0)
	{
		this.press(VK_END)
		return
	}
	local cur = this.selection
	if (cur[0] == dot && cur[1] == dot)
		this.press(VK_END)
	else
		this.selection = [dot, dot]
}

// ファイル名を選択 or 拡張子を選択
function editor_select()
{
	local text = this.text
	local dot = text.rfind(".")
	if (dot == null || dot == 0)
	{
		this.selection = [0, -1]
		return
	}
	local cur = this.selection
	if (cur[0] == 0 && cur[1] == dot)
		this.selection = [dot + 1, -1]
	else
		this.selection = [0, dot]
}

// ファイル名の先頭まで削除
function editor_del_to_home()
{
	press(VK_HOME | SHIFT)
	press(VK_DELETE)
}

// ファイル名の末尾まで削除
function editor_del_to_end()
{
	press(VK_END | SHIFT)
	press(VK_DELETE)
}

//=============================================================================
// Generic commands

function about()
{
	// WINDOWS_VERSION = {major}{minor}{NT}{SP}
	local WINDOWS_NAMES =
	{
		[0x060101] = "Server 2008 R2",
		[0x060100] = "7",
		[0x060001] = "Server 2008",
		[0x060000] = "Vista",
		[0x050202] = "Server 2003 R2",
		[0x050201] = "Server 2003",
		[0x050200] = "XP x64",
		[0x050100] = "XP",
		[0x050001] = "2000",
	}
	local windows = getslot(WINDOWS_NAMES, (WINDOWS_VERSION >> 8) & 0xFFFFFF) || "(Unknown)"
	local sp = WINDOWS_VERSION & 0xFF
	alert(format("%s %d.%d.%d.%d (build %s)",
			APPNAME,
			(VERSION >> 24) & 0xFF, (VERSION >> 16) & 0xFF, (VERSION >> 8) & 0xFF, VERSION & 0xFF,
			BUILD),
		format("with\n- %s\n- Windows %s (Service Pack %d)", SQUIRREL_VERSION, windows, sp),
		"バージョン情報")
}

function help()
{
	os.path("AVESTA/doc/index.html").execute()
}

//=============================================================================
// Default settings

import("default")

//=============================================================================
// Utils

function generate_hierarchy(template, parent = null)
{
	local obj = template.type(parent)

	foreach (key, value in template)
	{
		switch (key)
		{
		case "children":
		case "type":
			break
		default:
			obj[key] = value
		}
	}

	foreach (i in getslot(template, "children"))
		generate_hierarchy(i, obj)

	return obj
}

local function standard_call(target, fn)
{
	switch (typeof(fn))
	{
	case "null":
		return false
	case "string":
		target[fn]()
		return true
	default:
		fn.call(target)
		return true
	}
}

local function standard_keymap(map, key)
{
	return standard_call(this, getslot(map, key & KEYS))
}

local function standard_gesture(map, vk, dirs)
{
	return standard_call(this, getslot(map, vk, dirs))
}

//=============================================================================
// Settings

function settings_load()
{
	::settings <- {}

	try
	{
		import("settings")

		// form
		local placement = getslot(settings, "form", "placement")
		if (placement)
			form.placement = placement

		// config
		foreach (k, v in getslot(settings, "config"))
		{
			try { config[k] = v } catch (e) { }
		}

		// folders
		foreach (i in getslot(settings, "folders"))
		{
			try
			{
				local v = FolderView_new(os.path(i.path))
				v.visible = i.visible
			}
			catch (e)
			{
			}
		}
	}
	catch (e)
	{
		// ignore errors
	}

	::settings <- null
}

// called from Form.onClose
function settings_save()
{
	local settings = {}

	// form
	settings.form <- {}
	settings.form.placement <- this.placement

	// config
	settings.config <- {}
	foreach (k, v in config.getclass())
	{
		if (k == "get_keymap")
			continue
		if (k.startswith("get_"))
		{
			local key = k.slice(4)
			settings.config[key] <- v.call(config)
		}
	}

	// folders
	settings.folders <- []
	foreach (v in form.children)
	{
		if (v.dock != CENTER)
			continue
		settings.folders.append( { path = v.path.id, visible = v.visible } )
	}

	// write to file
	local out = Writer("AVESTA/lib/settings.nut")
	out.write("// saved at " + os.time())
	out.print("settings <- ");
	serialize(out, settings);
	out.write()
}

//=============================================================================

local function menu_for_array(children)
{
	local function fill(children)
	{
		foreach (i in children)
		{
			local name
			if (i == null)
				this.append(i, MF_SEPARATOR, null)
			else if ((name = getslot(i, "name")) != null)
				this.append(i, 0, name)
			else
				this.append(i, 0, i.tostring())
		}
	}

	local value = menu(fill, children)
	if (value != null)
		value.execute()
}

local function ToolBar_onDropDown(index)
{
	local item = this.items[index]
	local children = getslot(item, "children")
	if (children)
		menu_for_array(children)
}

local function ToolBar_onExecute(index)
{
	local item = this.items[index]
	local children = getslot(item, "children")
	if (children)
		menu_for_array(children)
	else
		item.execute()
}

local function is_visible_folder(path)
{
	return (path.attr & (SFGAO.HIDDEN | SFGAO.FOLDER)) == (SFGAO.FOLDER)
}

local function Path_onExecute(index)
{
	local item = this.items[index]
	local linked = getslot(item, "linked")
	if (linked)
		FolderView_goto(this.parent, item)
	else
		ToolBar_onDropDown(index)
}

local function Path_onDropDown(index)
{
	local item = this.items[index]
	local linked = getslot(item, "linked")
	if (linked)
	{
		local function fill(item)
		{
			foreach (i in item.get_children(is_visible_folder))
				this.append(i, 0, i.name)
		}

		local item = menu(fill, linked, this.menupos(index))
		if (item != null)
		{
			FolderView_goto(form.focus, item)
			return true
		}
	}
	else
		ToolBar_onDropDown(index)
}

//=============================================================================
// Buttons

class Button extends object
{
	view = null
	constructor(view)		{ this.view = view }
	function get_check()	{ return this.view.visible }
	function get_name()		{ return this.view.path.name }
	function get_icon()		{ return this.view.path.icon }
	function get_inner()	{ return this.view.path }
}

local function Buttons_update(exclude)
{
	if (::buttons && ::buttons.visible)
	{
		local folders = []
		foreach (v in form.children)
		{
			if (v == exclude || v.dock != CENTER)
				continue
			folders.append(Button(v))
		}
		::buttons.items = folders
	}
}

local function Buttons_toggle(item)
{
	local mods = modifiers()
	if (mods & RBUTTON)
	{
		local path = item.view.path
		if (!path.popup())
			FolderView_new(path)
	}
	else if (mods & MBUTTON)
		item.view.close()
	else
		item.view.visible = !item.view.visible
}
local function toggle_current(name)
{
	local target = form.focus
	if (target)
		target[name] = !target[name]
}

//=============================================================================
// FolderView

class FolderView extends ListView
{
	address = null

	function constructor(parent)
	{
		base.constructor(parent)
		address = ToolBar(this)
		address.height = 24
		address.onExecute = Path_onExecute
		address.onDropDown = Path_onDropDown
	}

	function onClose()
	{
		Buttons_update(this)
	}

	function onNavigate()
	{
		local view = this
		local path = this.path
		if (::tree && ::tree.visible)
			::tree.selection = path
		Buttons_update(null)

		local list = []

		// 先頭は便利メニュー with フォルダアイコン
		list.insert(0, {
			name = "",
			icon = path.icon,
			children = [
				{ name = "特大",  execute = @() view.mode = FVM.HUGE },
				{ name = "大",    execute = @() view.mode = FVM.LARGE },
				{ name = "中",    execute = @() view.mode = FVM.MEDIUM },
				{ name = "小",    execute = @() view.mode = FVM.SMALL },
				{ name = "一覧",  execute = @() view.mode = FVM.LIST },
				{ name = "詳細",  execute = @() view.mode = FVM.DETAILS },
				{ name = "並べる" execute = @() view.mode = FVM.TILE },
				null,
				{ name = "詳細ペイン"           , execute = @() view.detail = !view.detail },
				{ name = "プレビュー ペイン"    , execute = @() view.preview = !view.preview },
				{ name = "ナビゲーション ペイン", execute = @() view.nav = !view.nav },
			]
		})

		// 続けてフォルダ階層を追加。ただしルートは除外。
		while (path)
		{
			local parent = path.parent
			if (parent || list.len() <= 1)
			{	// 中間ノードはアイコンなし。
				list.insert(1, delegate(path, { icon = null }))
			}
			path = parent
		}

		address.items = list
	}

	function onExecute(items)
	{
		local moved = false
		local mods = modifiers()
		foreach (i in items)
		{
			local linked = i.linked
			if (!linked.browsable(false))
				FolderView_execute(i, mods)
			else if (moved)
				FolderView_new(linked)
			else
				moved = (FolderView_goto(this, linked) == this)
		}
		return true
	}
}

local FolderView_template =
{
	type = FolderView,
	onPress = bind(standard_keymap, FolderView_keymap),
	onGesture = bind(standard_gesture, FolderView_mouse),
}

function FolderView_new(path)
{
	if (!path)
		return null
	local target = generate_hierarchy(FolderView_template, ::form)
	try
	{
		target.path = path
		return target
	}
	catch (e)
	{
		target.close()
		return null
	}
}

function FolderView_goto(target, path)
{
	local mods = modifiers()
	if (typeof(path) == "string" && target)
		path = target.path.parse(path)
	else if ((mods & RBUTTON) && path.popup())
		return	// コンテキストメニューを処理した
	else
		path = path.linked

	// 表示できない場合は親を選択する
	local child = null
	while (path && !path.browsable(false))
	{
		child = path
		path = path.parent
	}
	if (!path)
		return null

	// Ctrl, Shift, 中クリックの場合は新しいビュー
	if (target == null || (mods & (CTRL | SHIFT | MBUTTON)))
		target = FolderView_new(path)
	else
		target.path = path

	// 項目を選択する
	if (target && child)
		target.selection = child

	return target
}

//=============================================================================
// FolderTree

class FolderTree extends TreeView
{
	function constructor(parent)
	{
		base.constructor(parent)
		append(os.path("Desktop"))
	}
}

//=============================================================================
// ToolBar

local function bind_config(name, value = null)
{
	return function()
	{
		config[name] = (value == null ? !config[name] : value)
	}
}

local ToolBar_items =
[
	{
		name = "操作",
		children = [
			{ name = "お気に入りに追加", execute = add_to_bookmark },
			{ name = "最大化", execute = @() ::form.maximize() },
			{ name = "最小化", execute = @() ::form.minimize() },
			{ name = "元に戻す", execute = @() ::form.restore() },
		]
	},

	{
		name = "設定",
		children = [
			{ name = "隠しファイルを表示", execute = bind_config("show_hidden_files") },
			{ name = "グリッドを表示", execute = bind_config("gridlines") },
			null,
			{ name = "Preview : 無効", execute = bind_config("floating_preview", FP.DISABLE) },
			{ name = "Preview : 選択", execute = bind_config("floating_preview", FP.SELECTION) },
			{ name = "Preview : 追従", execute = bind_config("floating_preview", FP.HOTTRACK) },
			null,
			{ name = "Pane : フォルダ"  , execute = bind_config("navigation_pane") },
			{ name = "Pane : 詳細"      , execute = bind_config("detail_pane") },
			{ name = "Pane : プレビュー", execute = bind_config("preview_pane") },
			null,
			{ name = "FVF : NAMESELECT"   , execute = bind_config("folderview_flag", FVF.NAMESELECT) },
			{ name = "FVF : FULLROWSELECT", execute = bind_config("folderview_flag", FVF.FULLROWSELECT) },
			{ name = "FVF : CHECKBOX"     , execute = bind_config("folderview_flag", FVF.CHECKBOX) },
			{ name = "FVF : CHECKBOX3"    , execute = bind_config("folderview_flag", FVF.CHECKBOX3) },
		]
	},

	{
		name = "ヘルプ",
		children = [
			{ name = "オンライン文書", execute = help },
			{ name = "バージョン情報", execute = about },
		]
	},
]

//=============================================================================
// Form

local function Form_onDrop(items)
{
	local parent = null
	local children = []

	foreach (i in items)
	{
		local child = null
		while (i && !i.browsable(false))
		{
			child = i
			i = i.parent
		}
		if (parent == null)
			parent = i;
		children.append(child)
	}

	if (parent)
		FolderView_new(parent).selection = children
}

const STR_SIDE = "サイドバー"
const STR_TREE = "フォルダツリー"
const STR_LINK = "リンクバー"
const STR_TOOL = "ツールバー"

local Form_template =
{
	type = Window,
	name = APPNAME,
	onPress = bind(standard_keymap, Form_keymap),
	onClose = settings_save,
	onDrop = Form_onDrop,
	children =
	[
		{
			type = ReBar,
			children =
			[
				{
					type = ToolBar,
					name = STR_TOOL,
					height = 24,
					items = ToolBar_items,
					onExecute = ToolBar_onExecute,
					onDropDown = ToolBar_onDropDown
				},
				{
					type = ToolBar,
					name = STR_LINK,
					height = 24,
					items = os.dir("Links"),
					onExecute = @(index) FolderView_goto(form.focus, items[index]),
					onDropDown = Path_onDropDown
				},
			]
		},
		{
			type = ToolBar,
			name = STR_SIDE,
			dock = WEST,
			width = 160,
			onExecute = @(index) Buttons_toggle(items[index]),
			children =
			[
				{
					type = FolderTree,
					name = STR_TREE,
					onExecute = @(item) FolderView_goto(form.focus, item),
				},
			]
		},
	]
}

function Form_new()
{
	local form = generate_hierarchy(Form_template)
	foreach (i in form.children)
	{
		if (i.name == STR_SIDE)
		{
			::buttons <- i
			foreach (j in i.children)
			{
				switch (j.name)
				{
				case STR_TREE:
					::tree <- j
					break
				}
			}
		}
	}
	return form
}

//=============================================================================
// Editor

// TODO: Should move ::editor to config.editor ?
editor <- function(hwnd)
{
	local view = TextView(hwnd)
	view.onPress = bind(standard_keymap, Editor_keymap)
	return view
}

//=============================================================================
// Overwrite default behaviors with config.nut.

try
{
	if (!import("config"))
	{
		// copy config.sample.nut to config.nut if not found.
		os.copy("AVESTA/lib/config.sample.nut", "AVESTA/lib/config.nut");
	}
}
catch (e)
{
	alert("ユーザスクリプトでエラーが発生 (config.nut)", e, null, ERROR)
}
