import("defs")

if (DEBUG)
	import("debug")

import("core")
form <- Form_new()
settings_load()
form.run()
