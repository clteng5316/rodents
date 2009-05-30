// Unnested array.
function unnest(...)
{
	local r = []
	foreach (i in vargv)
	{
		if (typeof(i) == "array")
			r.extend(i)
		else
			r.append(i)
	}
	return r;
}

// Create curried function with runtime 'this'.
function bind(fn, ...)
{
	local fixed = vargv
	return function(...)
	{
		return fn.acall(unnest(this, fixed, vargv))
	}
}

// Create curried function with fixed 'this'.
function bind_this(fn, self, ...)
{
	local fixed = unnest(self, vargv)
	return function(...)
	{
		return fn.acall(unnest(fixed, vargv))
	}
}

function map(iterable, fn)
{
	local result = []
	foreach (i in iterable)
		result.append(fn(i))
	return result
}

function quote(s)
{
	return "\"" + s + "\""
}

function escape(s)
{
	return s.replace("\\", "\\\\").replace("\"", "\\\"")
}

// Concat array
function concat(items, item, sep = "\r\n")
{
	if (items)
		return items + sep + item
	else
		return item
}

// Merge table
function merge(rhs)
{
	foreach (k, v in rhs)
		this[k] <- v
}

function serialize(out, value, prefix = "")
{
	switch (typeof(value))
	{
	case "array":
		local prefix2 = prefix + "\t"
		out.write("[")
		foreach (v in value)
		{
			out.print(prefix2)
			serialize(out, v, prefix2)
			out.write(",")
		}
		out.print(prefix + "]")
		break
	case "table":
		local prefix2 = prefix + "\t"
		out.write("{")
		foreach (k, v in value)
		{
			out.print(prefix2 + k + " = ")
			serialize(out, v, prefix2)
			out.write(",")
		}
		out.print(prefix + "}")
		break
	case "string":
		out.print(quote(escape(value)))
		break
	default:
		out.print(value.tostring())
		break
	}
}

class delegate
{
	inner = null
	attr = null

	function constructor(inner, attrs = null)
	{
		this.inner = inner
		foreach (k, v in attrs)
			this[k] = v
	}

	function _get(name)
	{
		if (name in this.attr)
			return this.attr[name]
		else
		{
			local r = this.inner[name]
			if (typeof(r) != "function")
				return r
			else
				return bind_this(r, this.inner)
		}
	}

	function _set(name, value)
	{
		if (this.attr == null)
			this.attr = {}
		this.attr[name] <- value
	}
}

//=============================================================================
// const

// LOG LEVEL
const ERROR   = 0x00000010
const WARNING = 0x00000030
const INFO    = 0x00000040

// SELECTION
const ALL		= -1
const CHECKED	= -2
const CHECKED1	= -3
const CHECKED2	= -4

// DIRECTION
const NONE		= 0
const NORTH		= 1
const SOUTH		= 2
const WEST		= 3
const EAST		= 4
const CENTER	= 5

// MB
const MB_OK                = 0x00000000
const MB_OKCANCEL          = 0x00000001
const MB_YESNOCANCEL       = 0x00000003
const MB_YESNO             = 0x00000004
const MB_RETRYCANCEL       = 0x00000005
const MB_CANCELTRYCONTINUE = 0x00000006

// MF
const MF_SEPARATOR      = 0x00000800
const MF_ENABLED        = 0x00000000
const MF_GRAYED         = 0x00000001
const MF_DISABLED       = 0x00000002
const MF_UNCHECKED      = 0x00000000
const MF_CHECKED        = 0x00000008
const MF_POPUP          = 0x00000010

// FVM : ListView.mode
enum FVM
{
	HUGE	= 7,	// 256 : FVM_THUMBSTRIP
	LARGE	= 5,	//  96 : FVM_THUMBNAIL
	MEDIUM	= 1,	//  48 : FVM_ICON
	SMALL	= 2,	//  16 : FVM_SMALLICON
	LIST	= 3,	//  16 : FVM_LIST
	DETAILS	= 4,	//  16 : FVM_DETAILS
	TILE	= 6,	//  48 : FVM_TILE
}

// FVF : Folder View Flags
enum FVF
{
	NAMESELECT,
	FULLROWSELECT,
	CHECKBOX,
	CHECKBOX3,
}

// FP : Floating Preview
enum FP
{
	DISABLE,
	SELECTION,
	HOTTRACK,
}

// SFGAO
enum SFGAO
{
	CANRENAME         = 0x00000010,
	CANDELETE         = 0x00000020,
	ENCRYPTED         = 0x00002000,
	ISSLOW            = 0x00004000,
	GHOSTED           = 0x00008000,
	LINK              = 0x00010000,
	SHARE             = 0x00020000,
	READONLY          = 0x00040000,
	HIDDEN            = 0x00080000,
	FILESYSANCESTOR   = 0x10000000,
	FOLDER            = 0x20000000,
	FILESYSTEM        = 0x40000000,
	REMOVABLE         = 0x02000000,
	COMPRESSED        = 0x04000000,
	BROWSABLE         = 0x08000000,
}

// MK
const LBUTTON  = 0x00000100
const RBUTTON  = 0x00000200
const SHIFT    = 0x00000400
const CTRL     = 0x00000800
const MBUTTON  = 0x00001000
const XBUTTON  = 0x00002000
const YBUTTON  = 0x00004000
const ALT      = 0x00008000
const WIN      = 0x00010000

const KEYS     = 0x00018CFF
const MODS     = 0x00018C00
const BUTTONS  = 0x00007300

// VK
const VK_BUTTON_L     = 0x01
const VK_BUTTON_R     = 0x02
const VK_BUTTON_M     = 0x04
const VK_BUTTON_X     = 0x05
const VK_BUTTON_Y     = 0x06
const VK_BUTTON_LL    = 0x97
const VK_BUTTON_RR    = 0x98
const VK_BUTTON_MM    = 0x99
const VK_BUTTON_XX    = 0x9A
const VK_BUTTON_YY    = 0x9B
const VK_WHEEL_LEFT   = 0x88
const VK_WHEEL_UP     = 0x89
const VK_WHEEL_RIGHT  = 0x8A
const VK_WHEEL_DOWN   = 0x8B

const VK_CANCEL       = 0x03
const VK_BACK         = 0x08
const VK_TAB          = 0x09
const VK_RETURN       = 0x0D

const VK_KANA         = 0x15
const VK_FINAL        = 0x18
const VK_KANJI        = 0x19
const VK_ESCAPE       = 0x1B
const VK_CONVERT      = 0x1C
const VK_NONCONVERT   = 0x1D
const VK_SPACE        = 0x20
const VK_PRIOR        = 0x21
const VK_NEXT         = 0x22
const VK_END          = 0x23
const VK_HOME         = 0x24
const VK_LEFT         = 0x25
const VK_UP           = 0x26
const VK_RIGHT        = 0x27
const VK_DOWN         = 0x28
const VK_SELECT       = 0x29
const VK_PRINT        = 0x2A
const VK_EXECUTE      = 0x2B
const VK_SNAPSHOT     = 0x2C
const VK_INSERT       = 0x2D
const VK_DELETE       = 0x2E
const VK_HELP         = 0x2F
const VK_F1           = 0x70
const VK_F2           = 0x71
const VK_F3           = 0x72
const VK_F4           = 0x73
const VK_F5           = 0x74
const VK_F6           = 0x75
const VK_F7           = 0x76
const VK_F8           = 0x77
const VK_F9           = 0x78
const VK_F10          = 0x79
const VK_F11          = 0x7A
const VK_F12          = 0x7B

// PROPERTYKEY
const PKEY_NAME  = "{B725F130-47EF-101A-A5F1-02608C9EEBAC}:10" // 名前
const PKEY_TYPE  = "{28636AA6-953D-11D2-B5D6-00C04FD918D0}:11" // 種類
const PKEY_SIZE  = "{B725F130-47EF-101A-A5F1-02608C9EEBAC}:12" // サイズ
const PKEY_ATTR  = "{B725F130-47EF-101A-A5F1-02608C9EEBAC}:13" // 属性
const PKEY_MTIME = "{B725F130-47EF-101A-A5F1-02608C9EEBAC}:14" // 更新日時
const PKEY_CTIME = "{B725F130-47EF-101A-A5F1-02608C9EEBAC}:15" // 作成日時
const PKEY_ATIME = "{B725F130-47EF-101A-A5F1-02608C9EEBAC}:16" // アクセス日時
const PKEY_OWNER = "{9B174B34-40FF-11D2-A27E-00C04FC30871}:4"  // 所有者

const PKEY_COMPUTER   = "{28636AA6-953D-11D2-B5D6-00C04FD918D0}:5" // コンピュータ
const PKEY_FILENAME   = "{41CF5AE0-F75A-4806-BD87-59C7D9248EB9}:100" // ファイル名
const PKEY_DATE       = "{F7DB74B4-4287-4103-AFBA-F1B13DCD75CF}:100" // 日付
const PKEY_FOLDERNAME = "{B725F130-47EF-101A-A5F1-02608C9EEBAC}:2" // フォルダ名
const PKEY_FOLDERPATH = "{E3E0584C-B788-4A5A-BB20-7F5A44C9ACDD}:6" // フォルダのパス
const PKEY_FOLDER     = "{DABD30ED-0043-4789-A7F8-D013A4736622}:100" // フォルダ
const PKEY_PATH       = "{E3E0584C-B788-4A5A-BB20-7F5A44C9ACDD}:7" // パス
const PKEY_KIND       = "{1E3EE840-BC2B-476C-8237-2ACD1A839B22}:3" // 分類
const PKEY_RAITING    = "{64440492-4C8B-11D1-8B70-080036B11A03}:9" // 評価

// コンピュータ
const PKEY_DRIVE_TYPE       = "{B725F130-47EF-101A-A5F1-02608C9EEBAC}:4"  // 種類
const PKEY_DRIVE_USAGE      = "{9B174B35-40FF-11D2-A27E-00C04FC30871}:5"  // 使用率
const PKEY_DRIVE_SPACE      = "{9B174B35-40FF-11D2-A27E-00C04FC30871}:2"  // 空き領域
const PKEY_DRIVE_SIZE       = "{9B174B35-40FF-11D2-A27E-00C04FC30871}:3"  // 合計サイズ
const PKEY_DRIVE_FILESYSTEM = "{9B174B35-40FF-11D2-A27E-00C04FC30871}:4"  // ファイルシステム
// ごみ箱
const PKEY_TRASH_WHERE      = "{9B174B33-40FF-11D2-A27E-00C04FC30871}:2"  // 元の場所
const PKEY_TRASH_WHEN       = "{9B174B33-40FF-11D2-A27E-00C04FC30871}:3"  // 削除日時
// コントロールパネル
const PKEY_CONTROL_CATEGORY = "{305CA226-D286-468E-B848-2B2E8E697B74}:2"  // カテゴリ

//=============================================================================
// Console

class Console extends TextView
{
	function onPress(key)
	{
		if (key == VK_DELETE)
		{
			this.text = null	// 履歴をすべて消す
			return true
		}
	}

	function onClose()
	{
		// 閉じずに隠すだけ
		this.visible = false
		return true
	}
}

console <- null

function stdout(str)
{
	if (console == null)
	{
		console = Console()
		console.name = APPNAME + ": Console"
		console.font = "メイリオ"
		console.size = [640, 480]
		console.readonly = true
	}
	if (!console.visible)
		console.visible = true
	console.append(str.replace("\n", "\r\n"))
}
