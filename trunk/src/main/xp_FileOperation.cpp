// xp_fileoperation.cpp

#include "stdafx.h"
#include "unknown.hpp"
#include "win32.hpp"
#include "string.hpp"

	//==========================================================================================================================
	// IFileOperationXp

	[ uuid("CA1BD5AA-C514-418B-B8CF-1926322967F1") ]
	__interface IFileOperationXp : IFileOperation
	{
		STDMETHODIMP RenameItem(PCWSTR item, PCWSTR name);
		STDMETHODIMP MoveItem(PCWSTR item, PCWSTR path);
		STDMETHODIMP MoveItems(size_t num, PCWSTR items[], PCWSTR folder);
		STDMETHODIMP CopyItem(PCWSTR item, PCWSTR path);
		STDMETHODIMP CopyItems(size_t num, PCWSTR items[], PCWSTR folder);
		STDMETHODIMP DeleteItem(PCWSTR item);
		STDMETHODIMP DeleteItems(size_t num, PCWSTR items[]);
		STDMETHODIMP NewItem(PCWSTR path);
	};

//==========================================================================================================================

namespace
{
	struct FileOp
	{
		UINT			what;
		PWSTR			from;
		PWSTR			to;
	};

	FileOp* NewFileOp(UINT what, PCWSTR from, PCWSTR to)
	{
		size_t	lnFrom = wcslen(from);
		size_t	lnTo = wcslen(to);
		size_t	ln = sizeof(FileOp);
		if (lnFrom > 0)
			ln += (lnFrom + 2) * sizeof(WCHAR);
		if (lnTo > 0)
			ln += (lnTo + 2) * sizeof(WCHAR);
		FileOp* op = (FileOp*)calloc(1, ln);
		op->what = what;
		BYTE* offset = (BYTE*)op + sizeof(FileOp);
		if (lnFrom > 0)
		{
			op->from = (PWSTR)offset;
			memcpy(op->from, from, lnFrom * sizeof(WCHAR));
			offset += (lnFrom + 2) * sizeof(WCHAR);
		}
		if (lnTo > 0)
		{
			op->to = (PWSTR)offset;
			memcpy(op->to, to, lnTo * sizeof(WCHAR));
			offset += (lnTo + 2) * sizeof(WCHAR);
		}
		return op;
	}

	class XpFileOperation : public Unknown< implements<IFileOperationXp, IFileOperation> >
	{
	private:
		HWND					m_hwndParent;
		std::vector<FileOp*>	m_ops;
		bool					m_aborted;

	public:
		~XpFileOperation();
		void Clear();

	public: // IFileOperation
		STDMETHODIMP Advise(IFileOperationProgressSink* sink, DWORD* pdwCookie)	{ return E_NOTIMPL; }
		STDMETHODIMP Unadvise(DWORD dwCookie)									{ return E_NOTIMPL; }
		STDMETHODIMP SetOperationFlags(DWORD dwOperationFlags)					{ return E_NOTIMPL; }
		STDMETHODIMP SetProgressMessage(PCWSTR message)						{ return E_NOTIMPL; }
		STDMETHODIMP SetProgressDialog(IOperationsProgressDialog* dlg)			{ return E_NOTIMPL; }
		STDMETHODIMP SetProperties(IPropertyChangeArray* pproparray)			{ return E_NOTIMPL; }
		STDMETHODIMP SetOwnerWindow(HWND hwndParent)							{ m_hwndParent = hwndParent; return S_OK; }
		STDMETHODIMP ApplyPropertiesToItem(IShellItem* item)					{ return E_NOTIMPL; }
		STDMETHODIMP ApplyPropertiesToItems(IUnknown* items)					{ return E_NOTIMPL; }
		STDMETHODIMP RenameItem(IShellItem* item, PCWSTR name, IFileOperationProgressSink* sink);
		STDMETHODIMP RenameItems(IUnknown* items, PCWSTR name);
		STDMETHODIMP MoveItem(IShellItem* item, IShellItem* folder, PCWSTR name, IFileOperationProgressSink* sink);
		STDMETHODIMP MoveItems(IUnknown* items, IShellItem* folder);
		STDMETHODIMP CopyItem(IShellItem* item, IShellItem* folder, PCWSTR name, IFileOperationProgressSink* sink);
		STDMETHODIMP CopyItems(IUnknown* items, IShellItem* folder);
		STDMETHODIMP DeleteItem(IShellItem* item, IFileOperationProgressSink* sink);
		STDMETHODIMP DeleteItems(IUnknown* items);
		STDMETHODIMP NewItem(IShellItem* folder, DWORD attr, PCWSTR name, PCWSTR pszTemplateName, IFileOperationProgressSink* sink);
		STDMETHODIMP PerformOperations();
		STDMETHODIMP GetAnyOperationsAborted(BOOL *pfAnyOperationsAborted);

	public: // IFileOperationXp
		STDMETHODIMP RenameItem(PCWSTR item, PCWSTR name);
		STDMETHODIMP MoveItem(PCWSTR item, PCWSTR path);
		STDMETHODIMP MoveItems(size_t num, PCWSTR items[], PCWSTR folder);
		STDMETHODIMP CopyItem(PCWSTR item, PCWSTR path);
		STDMETHODIMP CopyItems(size_t num, PCWSTR items[], PCWSTR folder);
		STDMETHODIMP DeleteItem(PCWSTR item);
		STDMETHODIMP DeleteItems(size_t num, PCWSTR items[]);
		STDMETHODIMP NewItem(PCWSTR path);
	};

	bool ChoiceBetter(PWSTR bestfile, PCWSTR candidate)
	{
		bool better = false;
		if (str::empty(bestfile))
		{
			better = true;
		}
		else
		{
			int i;
			for (i = 0;
				bestfile[i] != L'\0' && candidate[i] != L'\0'
				&& tolower(bestfile[i]) == tolower(candidate[i]);
				++i)
			{ }
			better = (_wtoi(bestfile+i) < _wtoi(candidate+i));
		}
		//
		if (better)
		{
			wcscpy_s(bestfile, MAX_PATH, candidate);
			return true;
		}
		return false;
	}

	HRESULT FileNewShell(XpFileOperation* op, PCWSTR path, PCWSTR ext, PCWSTR templates)
	{
		WCHAR reg[MAX_PATH], srcfile[MAX_PATH] = L"";
		wcscpy_s(reg, ext);
		wcscat_s(reg, L"\\ShellNew");
		// レジストリの単純ShellNewエントリを探す
		if SUCCEEDED(RegGetString(srcfile, HKEY_CLASSES_ROOT, reg, L"FileName"))
		{
			if (PathIsRelative(srcfile))
				PathCombine(srcfile, templates, srcfile);
			if (PathFileExists(srcfile))
				return op->CopyItem(srcfile, path);
		}
		return E_FAIL;
	}

	HRESULT FileNewTemplate(XpFileOperation* op, PCWSTR path, PCWSTR ext, PCWSTR templates)
	{
		WCHAR wildcard[MAX_PATH], srcfile[MAX_PATH] = L"";
		WIN32_FIND_DATA find;
		wcscpy_s(wildcard, templates);
		wcscat_s(wildcard, L"*");
		wcscat_s(wildcard, ext);
		HANDLE hFind = ::FindFirstFile(wildcard, &find);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				ChoiceBetter(srcfile, find.cFileName);
			} while (::FindNextFile(hFind, &find));
			::FindClose(hFind);
			if (!str::empty(srcfile))
			{
				PathCombine(srcfile, templates, srcfile);
				return op->CopyItem(srcfile, path);
			}
		}
		return E_FAIL;
	}

	HRESULT NewEmptyFile(PCWSTR path)
	{
		HANDLE hFile = ::CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE)
			return HRESULT_FROM_WIN32(GetLastError());
		::SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, path, NULL);
		::CloseHandle(hFile);
		return S_OK;
	}
}

//==========================================================================================================================

XpFileOperation::~XpFileOperation()
{
	Clear();
}

void XpFileOperation::Clear()
{
	for (size_t i = 0; i < m_ops.size(); i++)
		free(m_ops[i]);
	m_ops.clear();
}

STDMETHODIMP XpFileOperation::RenameItem(IShellItem* item, PCWSTR name, IFileOperationProgressSink* sink)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::RenameItems(IUnknown* items, PCWSTR name)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::MoveItem(IShellItem* item, IShellItem* folder, PCWSTR name, IFileOperationProgressSink* sink)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::MoveItems(IUnknown* items, IShellItem* folder)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::CopyItem(IShellItem* item, IShellItem* folder, PCWSTR name, IFileOperationProgressSink* sink)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::CopyItems(IUnknown* items, IShellItem* folder)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::DeleteItem(IShellItem* item, IFileOperationProgressSink* sink)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::DeleteItems(IUnknown* items)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::NewItem(IShellItem* folder, DWORD attr, PCWSTR name, PCWSTR pszTemplateName, IFileOperationProgressSink* sink)
{
	return E_NOTIMPL;
}

STDMETHODIMP XpFileOperation::PerformOperations()
{
	m_aborted = false;
	SHFILEOPSTRUCT op = { m_hwndParent };
	for (size_t i = 0; i < m_ops.size(); i++)
	{
		if (m_ops[i]->what == FO_COPY && str::empty(m_ops[i]->from))
		{
			if FAILED(NewEmptyFile(m_ops[i]->to))
				m_aborted = true;
		}
		else
		{
			op.wFunc = m_ops[i]->what;
			op.pFrom = m_ops[i]->from;
			op.pTo = m_ops[i]->to;
			op.fFlags = FOF_ALLOWUNDO;
			SHFileOperation(&op);
			if (op.fAnyOperationsAborted)
				m_aborted = true;
		}
	}
	Clear();
	return S_OK;
}

STDMETHODIMP XpFileOperation::GetAnyOperationsAborted(BOOL *pfAnyOperationsAborted)
{
	*pfAnyOperationsAborted = m_aborted;
	return S_OK;
}

STDMETHODIMP XpFileOperation::RenameItem(PCWSTR item, PCWSTR name)
{
	m_ops.push_back(NewFileOp(FO_RENAME, item, name));
	return S_OK;
}

STDMETHODIMP XpFileOperation::MoveItem(PCWSTR item, PCWSTR path)
{
	m_ops.push_back(NewFileOp(FO_MOVE, item, path));
	return S_OK;
}

STDMETHODIMP XpFileOperation::MoveItems(size_t num, PCWSTR items[], PCWSTR folder)
{
	m_ops.push_back(NewFileOp(FO_MOVE, items[0], folder));
	return S_OK;
}

STDMETHODIMP XpFileOperation::CopyItem(PCWSTR item, PCWSTR path)
{
	m_ops.push_back(NewFileOp(FO_COPY, item, path));
	return S_OK;
}

STDMETHODIMP XpFileOperation::CopyItems(size_t num, PCWSTR items[], PCWSTR folder)
{
	m_ops.push_back(NewFileOp(FO_COPY, items[0], folder));
	return S_OK;
}

STDMETHODIMP XpFileOperation::DeleteItem(PCWSTR item)
{
	m_ops.push_back(NewFileOp(FO_DELETE, item, null));
	return S_OK;
}

STDMETHODIMP XpFileOperation::DeleteItems(size_t num, PCWSTR items[])
{
	m_ops.push_back(NewFileOp(FO_DELETE, items[0], null));
	return S_OK;
}

STDMETHODIMP XpFileOperation::NewItem(PCWSTR path)
{
	PCWSTR ext = PathFindExtension(path);
	if (*ext == L'.')
	{
		WCHAR templates[MAX_PATH];
		SHGetSpecialFolderPath(NULL, templates, CSIDL_TEMPLATES, FALSE);
		PathAddBackslash(templates);
		// まずは単純ShellNewエントリを参照する。
		if SUCCEEDED(FileNewShell(this, path, ext, templates))
			return S_OK;
		// 次にCSIDL_TEMPLATESフォルダの同じ拡張子を捜す
		if SUCCEEDED(FileNewTemplate(this, path, ext, templates))
			return S_OK;
	}
	// テンプレートファイルが無い
	return CopyItem(null, path);
}

//==========================================================================================================================

HRESULT XpFileOperationCreate(REFINTF pp)
{
	return newobj<XpFileOperation>(pp);
}
