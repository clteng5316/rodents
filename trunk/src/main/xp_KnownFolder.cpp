// xp_knownfolder.cpp

#include "stdafx.h"
#include "unknown.hpp"
#include "win32.hpp"
#include "string.hpp"

//==========================================================================================================================

namespace
{
	const static KNOWNFOLDERID IDs[] =
	{
		FOLDERID_AdminTools,
		FOLDERID_CDBurning,
		FOLDERID_CommonAdminTools,
		FOLDERID_CommonPrograms,
		FOLDERID_CommonStartMenu,
		FOLDERID_CommonStartup,
		FOLDERID_CommonTemplates,
		FOLDERID_ComputerFolder,
		FOLDERID_ConnectionsFolder,
		FOLDERID_Contacts,
		FOLDERID_ControlPanelFolder,
		FOLDERID_Cookies,
		FOLDERID_Desktop,
		FOLDERID_Documents,
		FOLDERID_Downloads,
		FOLDERID_Favorites,
		FOLDERID_Fonts,
		FOLDERID_Games,
		FOLDERID_GameTasks,
		FOLDERID_History,
		FOLDERID_InternetCache,
		FOLDERID_InternetFolder,
		FOLDERID_Links,
		FOLDERID_LocalAppData,
		FOLDERID_LocalAppDataLow,
		FOLDERID_LocalizedResourcesDir,
		FOLDERID_Music,
		FOLDERID_NetHood,
		FOLDERID_NetworkFolder,
		FOLDERID_Pictures,
		FOLDERID_Playlists,
		FOLDERID_PrintersFolder,
		FOLDERID_PrintHood,
		FOLDERID_Profile,
		FOLDERID_ProgramData,
		FOLDERID_ProgramFiles,
		FOLDERID_ProgramFilesCommon,
		FOLDERID_Programs,
		FOLDERID_Public,
		FOLDERID_PublicDesktop,
		FOLDERID_PublicDocuments,
		FOLDERID_PublicDownloads,
		FOLDERID_PublicGameTasks,
		FOLDERID_PublicMusic,
		FOLDERID_PublicPictures,
		FOLDERID_PublicVideos,
		FOLDERID_QuickLaunch,
		FOLDERID_Recent,
		FOLDERID_RecordedTV,
		FOLDERID_RecycleBinFolder,
		FOLDERID_ResourceDir,
		FOLDERID_RoamingAppData,
		FOLDERID_SampleMusic,
		FOLDERID_SamplePictures,
		FOLDERID_SamplePlaylists,
		FOLDERID_SampleVideos,
		FOLDERID_SavedGames,
		FOLDERID_SavedSearches,
		FOLDERID_SEARCH_CSC,
		FOLDERID_SEARCH_MAPI,
		FOLDERID_SearchHome,
		FOLDERID_SendTo,
		FOLDERID_StartMenu,
		FOLDERID_Startup,
		FOLDERID_SyncManagerFolder,
		FOLDERID_SyncResultsFolder,
		FOLDERID_SyncSetupFolder,
		FOLDERID_System,
		FOLDERID_Templates,
		FOLDERID_TreeProperties,
		FOLDERID_UserProfiles,
		FOLDERID_UsersFiles,
		FOLDERID_Videos,
		FOLDERID_Windows,
		GUID_NULL,
		GUID_NULL,
		GUID_NULL,
		GUID_NULL,
		GUID_NULL,
		GUID_NULL,
		GUID_NULL,
		GUID_NULL,
	};

	const static struct KnownFolderDesc
	{
		int				csidl;
		PCWSTR			name;
		PCWSTR			path;
	}
	DESCs[] = 
	{
		{ CSIDL_ADMINTOOLS	, L"Administrative Tools"},	// FOLDERID_Programs
		{ CSIDL_CDBURN_AREA	, L"CD Burning"},	// FOLDERID_LocalAppData
		{ CSIDL_COMMON_ADMINTOOLS	},
		{ CSIDL_COMMON_PROGRAMS	},
		{ CSIDL_COMMON_STARTMENU	},
		{ CSIDL_COMMON_STARTUP	},
		{ CSIDL_COMMON_TEMPLATES	},
		{ CSIDL_DRIVES		, L"MyComputerFolder" , FOLDER_COMPUTER},
		{ CSIDL_CONNECTIONS	, L"ConnectionsFolder" },
		{ -1 },
		{ CSIDL_CONTROLS	, L"ControlPanelFolder"	, FOLDER_CONTROL },
		{ CSIDL_COOKIES		, L"Cookies" },
		{ CSIDL_DESKTOP		, L"Desktop" },
		{ CSIDL_MYDOCUMENTS	, L"Personal", FOLDER_PERSONAL },	// FOLDERID_Profile
		{ -1, L"Downloads" },	// FOLDERID_Profile/Downloads
		{ CSIDL_FAVORITES	, L"Favorites" },	// FOLDERID_Profile/Favorites
		{ CSIDL_FONTS		, L"Fonts" },	// FOLDERID_Windows
		{ CSIDL_GAMES },
		{ -1 },
		{ CSIDL_HISTORY		, L"History" },	// FOLDERID_LocalAppData
		{ CSIDL_INTERNET_CACHE	},
		{ CSIDL_INTERNET	},
		{ -1, L"Links", L"links" },	//FOLDERID_Profile/Links
		{ CSIDL_LOCAL_APPDATA	},
		{ -1 },
		{ CSIDL_RESOURCES_LOCALIZED	},
		{ CSIDL_MYMUSIC		, L"My Music" },	// FOLDERID_Profile/Music
		{ CSIDL_NETHOOD		, L"Nethood" },
		{ CSIDL_NETWORK		, L"Network", FOLDER_NETWORK },
		{ CSIDL_MYPICTURES	, L"My Pictures" },	// FOLDERID_Profile : ::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\{FOLDERID_Pictures}
		{ -1 },
		{ CSIDL_PRINTERS	},
		{ CSIDL_PRINTHOOD	},
		{ CSIDL_PROFILE			, L"Profile",		FOLDER_PROFILE },
		{ CSIDL_PROGRAM_FILES	, L"Common AppData" },
		{ CSIDL_PROGRAM_FILES	, L"ProgramFiles" },
		{ CSIDL_PROGRAM_FILES_COMMON	},
		{ CSIDL_PROGRAMS	},
		{ -1 },
		{ CSIDL_COMMON_DESKTOPDIRECTORY	},
		{ CSIDL_COMMON_DOCUMENTS	},
		{ -1 },
		{ -1 },
		{ CSIDL_COMMON_MUSIC	},
		{ CSIDL_COMMON_PICTURES	},
		{ CSIDL_COMMON_VIDEO	},
		{ -1 },
		{ CSIDL_RECENT	},
		{ -1 },
		{ CSIDL_BITBUCKET	, L"RecycleBinFolder", FOLDER_TRASH },
		{ CSIDL_RESOURCES	},
		{ -1 },
		{ -1 },
		{ -1 },
		{ -1 },
		{ -1 },
		{ -1 },
		{ -1 },
		{ -1 },
		{ -1 },
		{ -1 },
		{ CSIDL_SENDTO	},
		{ CSIDL_STARTMENU	},
		{ CSIDL_STARTUP	},
		{ -1 },
		{ -1 },
		{ -1 },
		{ CSIDL_SYSTEM	},
		{ CSIDL_TEMPLATES	},
		{ -1 },
		{ CSIDL_PERSONAL	},
		{ -1 },
		{ CSIDL_MYVIDEO	},
		{ CSIDL_WINDOWS				, L"Windows" },
		{ CSIDL_ALTSTARTUP	},
		{ CSIDL_APPDATA				},
		{ CSIDL_COMMON_ALTSTARTUP	},
		{ CSIDL_COMMON_APPDATA		},
		{ CSIDL_COMMON_FAVORITES	},
		{ CSIDL_COMMON_OEM_LINKS	},
		{ CSIDL_COMPUTERSNEARME		},
		{ CSIDL_DESKTOPDIRECTORY	},
	};

	class XpKnownFolder : public Unknown< implements<IKnownFolder> >
	{
	private:
		const KnownFolderDesc& m_desc;

	public:
		XpKnownFolder(const KnownFolderDesc& desc);

	public: // IKnownFolder
		IFACEMETHODIMP GetId(KNOWNFOLDERID *pkfid);
		IFACEMETHODIMP GetCategory(KF_CATEGORY *pCategory);
		IFACEMETHODIMP GetShellItem(DWORD dwFlags, REFIID riid, void **pp);
		IFACEMETHODIMP GetPath(DWORD dwFlags, LPWSTR *ppszPath);
		IFACEMETHODIMP SetPath(DWORD dwFlags, PCWSTR pszPath);
		IFACEMETHODIMP GetIDList(DWORD dwFlags, PIDLIST_ABSOLUTE *ppidl);
		IFACEMETHODIMP GetFolderType(FOLDERTYPEID *pftid);
		IFACEMETHODIMP GetRedirectionCapabilities(KF_REDIRECTION_CAPABILITIES *pCapabilities);
		IFACEMETHODIMP GetFolderDefinition(KNOWNFOLDER_DEFINITION *pKFD);

	private:
		XpKnownFolder(const XpKnownFolder&);
		XpKnownFolder& operator = (const XpKnownFolder&);
	};

	class XpKnownFolderManager : public Unknown< implements<IKnownFolderManager>, mixin<StaticLife> >
	{
	public: // IKnownFolderManager
		IFACEMETHODIMP FolderIdFromCsidl(int nCsidl, KNOWNFOLDERID *pfid)		{ return E_NOTIMPL; }
		IFACEMETHODIMP FolderIdToCsidl(REFKNOWNFOLDERID rfid, int *pnCsidl)	{ return E_NOTIMPL; }
		IFACEMETHODIMP GetFolderIds(KNOWNFOLDERID **ppKFId, UINT *pCount);
		IFACEMETHODIMP GetFolder(REFKNOWNFOLDERID rfid, IKnownFolder **pp);
		IFACEMETHODIMP GetFolderByName(PCWSTR name, IKnownFolder **pp);
		IFACEMETHODIMP RegisterFolder(REFKNOWNFOLDERID rfid, const KNOWNFOLDER_DEFINITION *pKFD)	{ return E_NOTIMPL; }
		IFACEMETHODIMP UnregisterFolder(REFKNOWNFOLDERID rfid)									{ return E_NOTIMPL; }
		IFACEMETHODIMP FindFolderFromPath(PCWSTR pszPath, FFFP_MODE mode, IKnownFolder **pp)	{ return E_NOTIMPL; }
		IFACEMETHODIMP FindFolderFromIDList(PCIDLIST_ABSOLUTE pidl, IKnownFolder **pp)			{ return E_NOTIMPL; }
		IFACEMETHODIMP Redirect(REFKNOWNFOLDERID rfid, HWND hwnd, KF_REDIRECT_FLAGS flags, PCWSTR pszTargetPath, UINT cFolders, const KNOWNFOLDERID *pExclusion, LPWSTR *ppszError)	{ return E_NOTIMPL; }
	};

	XpKnownFolderManager theXpKnownFolderManager;

	STATIC_ASSERT( lengthof(IDs) == lengthof(DESCs) );
}

//==========================================================================================================================

IFACEMETHODIMP XpKnownFolderManager::GetFolderIds(KNOWNFOLDERID **ppKFId, UINT *pCount)
{
	*ppKFId = const_cast<KNOWNFOLDERID*>(IDs);
	*pCount = lengthof(IDs);
	return S_OK;
}

IFACEMETHODIMP XpKnownFolderManager::GetFolder(REFKNOWNFOLDERID rfid, IKnownFolder **pp)
{
	if (!pp)
		return E_POINTER;
	*pp = null;

	for (size_t i = 0; i < lengthof(DESCs); ++i)
	{
		if (IDs[i] == rfid)
		{
			*pp = new XpKnownFolder(DESCs[i]);
			break;
		}
	}

	return *pp ? S_OK : E_INVALIDARG;
}

IFACEMETHODIMP XpKnownFolderManager::GetFolderByName(PCWSTR name, IKnownFolder **pp)
{
	if (!pp)
		return E_POINTER;
	*pp = null;

	for (size_t i = 0; i < lengthof(DESCs); ++i)
	{
		if (DESCs[i].name && _wcsicmp(name, DESCs[i].name) == 0)
		{
			*pp = new XpKnownFolder(DESCs[i]);
			break;
		}
	}

	return *pp ? S_OK : E_INVALIDARG;
}

//==========================================================================================================================

XpKnownFolder::XpKnownFolder(const KnownFolderDesc& desc) : m_desc(desc)
{
}

IFACEMETHODIMP XpKnownFolder::GetId(KNOWNFOLDERID *pkfid)
{
	*pkfid = IDs[&m_desc - DESCs];
	return S_OK;
}

IFACEMETHODIMP XpKnownFolder::GetCategory(KF_CATEGORY *pCategory)
{
//	*pCategory = m_desc.category;
	ASSERT(0);
	return E_NOTIMPL;
}

IFACEMETHODIMP XpKnownFolder::GetShellItem(DWORD dwFlags, REFIID riid, void **pp)
{
	HRESULT	hr;
	ILPtr id;
	ref<IShellItem> item;
	if (FAILED(hr = GetIDList(0, &id)) ||
		FAILED(hr = PathCreate(&item, id)))
		return hr;
	return item->QueryInterface(riid, pp);
}

IFACEMETHODIMP XpKnownFolder::GetPath(DWORD dwFlags, LPWSTR *ppszPath)
{
	if (!ppszPath)
		return E_POINTER;

	WCHAR	path[MAX_PATH];

	if (m_desc.csidl >= 0 && SHGetSpecialFolderPath(GetWindow(), path, m_desc.csidl, false))
		;
	else if (m_desc.path && m_desc.path[0] != L':')
		PathCombine(path, PATH_ROOT, m_desc.path);
	else
		return E_FAIL;

	*ppszPath = str::dup<CoTaskMem>(path).detach();
	return S_OK;
}

IFACEMETHODIMP XpKnownFolder::SetPath(DWORD dwFlags, PCWSTR pszPath)
{
	ASSERT(0);
	return E_NOTIMPL;
}

IFACEMETHODIMP XpKnownFolder::GetIDList(DWORD dwFlags, PIDLIST_ABSOLUTE *ppidl)
{
	if (m_desc.csidl >= 0)
		return ::SHGetSpecialFolderLocation(GetWindow(), m_desc.csidl, ppidl);

	if (!m_desc.path)
		return E_FAIL;

	if (m_desc.path[0] == L':')
		return ::SHILCreateFromPath(m_desc.path, ppidl, null);

	WCHAR	path[MAX_PATH];
	PathCombine(path, PATH_ROOT, m_desc.path);
	return ::SHILCreateFromPath(path, ppidl, null);
}

IFACEMETHODIMP XpKnownFolder::GetFolderType(FOLDERTYPEID *pftid)
{
	ASSERT(0);
	return E_NOTIMPL;
}

IFACEMETHODIMP XpKnownFolder::GetRedirectionCapabilities(KF_REDIRECTION_CAPABILITIES *pCapabilities)
{
	ASSERT(0);
	return E_NOTIMPL;
}

IFACEMETHODIMP XpKnownFolder::GetFolderDefinition(KNOWNFOLDER_DEFINITION *pKFD)
{
	ASSERT(0);
	return E_NOTIMPL;
}

//==========================================================================================================================

HRESULT XpKnownFolderManagerCreate(REFINTF pp)
{
	return theXpKnownFolderManager.QueryInterface(pp);
}
