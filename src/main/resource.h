#pragma once

// Fix those version numbers for each release.
#define VERSION_MAJOR		0
#define VERSION_MINOR		0
#define VERSION_RELEASE		0
#define VERSION_REVISION	2


#define MAKE_VERSION_NUM(a, b, c, d)	a,b,c,d
#define MAKE_VERSION_STR(a, b, c, d)	#a ", " #b ", " #c ", " #d "\0" 

#define	VERSION_NUM		MAKE_VERSION_NUM(VERSION_MAJOR, VERSION_MINOR, VERSION_RELEASE, VERSION_REVISION)
#define VERSION_STR		MAKE_VERSION_STR(VERSION_MAJOR, VERSION_MINOR, VERSION_RELEASE, VERSION_REVISION)

#define STR_APPNAME		1000
#define STR_EMPTY		1001
#define STR_GO			1002
#define STR_LOCKBARS	1003
#define STR_TILEHORZ	1004
#define STR_TILEVERT	1005
#define STR_SCRIPTERROR	1006
#define STR_DESKTOP		1007
#define STR_CALLSTACK	1008
#define STR_SRCFORMAT	1009
#define STR_OK			1010
#define STR_CANCEL		1011
#define STR_BEFORE		1012
#define STR_AFTER		1013
#define STR_REVERT		1014
#define STR_EXCLUDE		1015
#define STR_RENAME_EDIT	1016

#define STR_E_NOENT		2001
#define STR_E_ACCESS	2002
#define STR_E_NOMEM		2003
#define STR_E_NODLL		2004
#define STR_E_NOACTION	2005
#define STR_E_DDE		2006
#define STR_E_NOEXEC	2007
#define STR_E_SUPPORT	2008
#define STR_E_DIRECTORY	2009
#define STR_E_NOFILE	2010
#define STR_E_SHORTCUT	2011
#define STR_E_COMPILE	2012
