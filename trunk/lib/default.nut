function FolderView_execute(item, mods)
{
	if (mods & SHIFT)
		item.execute(".txt")
	else if (mods & (CTRL | MBUTTON))
		item.execute("edit")
	else
		item.execute()
}

config.keymap =
{
	merge = ::merge,
	[ VK_NONCONVERT     ] = @() form.visible = !form.visible,
	[ 'A' | WIN         ] = @() form.visible = !form.visible,
	[ 'A' | WIN | SHIFT ] = @() form.visible = !form.visible,
}

Form_keymap <-
{
	merge = ::merge,
	[ VK_F1              ] = about,
	[ VK_F12             ] = os.settings,
	[ 'Q' | CTRL         ] = "close",
}

FolderView_keymap <-
{
	merge = ::merge,
	[ VK_BACK            ] = go_up,
	[ VK_BACK | CTRL     ] = go_back,
	[ VK_BACK | SHIFT    ] = go_back,
	[ VK_DELETE          ] = "remove",
	[ VK_DELETE | SHIFT  ] = "bury",
	[ VK_ESCAPE          ] = @() set_selection(null),
	[ VK_F2              ] = "rename",
//	[ VK_F3              ] = "find",
//	[ VK_F4              ] = "address bar to string",
	[ VK_F5              ] = "refresh",
//	[ VK_F6              ] = "focus next ui object",
	[ VK_UP     | ALT    ] = go_up,
	[ VK_LEFT   | ALT    ] = go_back,
	[ VK_RIGHT  | ALT    ] = go_forward,
	[ 'A' | CTRL         ] = @() set_selection(ALL),
	[ 'B' | SHIFT        ] = bind(execute_dir, "cmd.exe"),
	[ 'C' | CTRL         ] = "copy",
	[ 'C' | CTRL | ALT   ] = copy_path,
	[ 'C' | CTRL | SHIFT ] = copy_name,
	[ 'D' | CTRL         ] = "remove",
	[ 'D' | CTRL | SHIFT ] = "bury",
	[ 'E' | CTRL         ] = "rename",
	[ 'E' | CTRL | SHIFT ] = rename_dialog,
	[ 'E' | SHIFT        ] = rename_dialog,
//	[ 'F' | CTRL         ] = "find",
	[ 'J' | CTRL         ] = "adjust",
	[ 'N' | CTRL         ] = "newfolder",
	[ 'N' | SHIFT        ] = "newfile",
	[ 'S' | CTRL         ] = @() set_selection(CHECKED1),
	[ 'S' | SHIFT        ] = @() set_selection(CHECKED2),
	[ 'S' | CTRL | SHIFT ] = @() set_selection(CHECKED),
	[ 'T' | CTRL         ] = @() FolderTree_toggle(path),
	[ 'T' | SHIFT        ] = @() FolderTree_only(path),
	[ 'R' | CTRL         ] = "refresh",
	[ 'V' | CTRL         ] = "paste",
	[ 'V' | CTRL | SHIFT ] = paste_name,
	[ 'V' | SHIFT        ] = paste_into,
	[ 'W' | CTRL         ] = "close",
	[ 'X' | CTRL         ] = "cut",
	[ 'Z' | CTRL         ] = "undo",
}

FolderView_mouse <-
{
	merge = ::merge,
	[ VK_WHEEL_UP  ] = { [ "" ] = go_up, },
	[ VK_BUTTON_L  ] = { [ "" ] = go_up, },
	[ VK_BUTTON_LL ] = { [ "" ] = go_up, },
	[ VK_BUTTON_M  ] =
	{
		[ "" ] = go_up,
		U = go_up,
		L = go_back,
		R = go_forward,
		D = "close",
	},
	[ VK_BUTTON_R  ] =
	{
		U = go_up,
		L = go_back,
		R = go_forward,
		D = "close",
	},
}

Editor_keymap <-
{
	merge = ::merge,
	[ 'A' | CTRL ] = editor_home,
	[ 'B' | CTRL ] = @() press(VK_LEFT),
	[ 'D' | CTRL ] = @() press(VK_DELETE),
	[ 'E' | CTRL ] = editor_end,
	[ 'F' | CTRL ] = @() press(VK_RIGHT),
	[ 'K' | CTRL ] = editor_del_to_end,
	[ 'N' | CTRL ] = @() press(VK_DOWN),
	[ 'P' | CTRL ] = @() press(VK_UP),
	[ 'U' | CTRL ] = editor_del_to_home,
	[ 'S' | CTRL ] = editor_select,
}

RenameDialog_keymap <-
{
	merge = ::merge,
	[ 'W' | CTRL         ] = "close",
}
