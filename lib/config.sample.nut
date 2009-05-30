// config.nut
//
// 以下の処理を修正してください。
// - config.keymap
// - Form_keymap
// - FolderView_execute(item, mods)
// - FolderView_keymap
// - FolderView_mouse
// - Editor_keymap

//=============================================================================
// config.keymap

/*
delete config.keymap[VK_NONCONVERT]
*/

//=============================================================================
// FolderView_keymap

/*
local Notepad = "WINDOWS/notepad.exe"

FolderView_keymap.merge({
	[ 'B' | CTRL ] = bind(execute_one, Notepad),
})
*/
