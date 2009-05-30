// xp_ShellItemArray.cpp

#include "stdafx.h"
#include "unknown.hpp"
#include "win32.hpp"

//==========================================================================================================================

namespace
{
	class XpShellItemArray : public Unknown< implements<IShellItemArray> >
	{
	protected:
		std::vector< ref<IShellItem> >	m_items;

	public:
		XpShellItemArray(const CIDA* cida, IShellFolder* folder);
		XpShellItemArray(const ITEMIDLIST* parent, IShellFolder* folder, size_t count, ITEMIDLIST** children);

	public: // IShellItemArray
        IFACEMETHODIMP BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **pp);
        IFACEMETHODIMP GetPropertyStore(GETPROPERTYSTOREFLAGS flags, REFIID riid, void **pp);
        IFACEMETHODIMP GetPropertyDescriptionList(REFPROPERTYKEY keyType, REFIID riid, void **pp);
        IFACEMETHODIMP GetAttributes(SIATTRIBFLAGS dwAttribFlags, SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs);
        IFACEMETHODIMP GetItemAt(DWORD index, IShellItem **pp);
        IFACEMETHODIMP GetCount(DWORD* outCount);
        IFACEMETHODIMP EnumItems(IEnumShellItems** pp);
	};
}

XpShellItemArray::XpShellItemArray(const CIDA* cida, IShellFolder* folder_)
{
	ref<IShellFolder> folder(folder_);
	size_t count = CIDACount(cida);
	const ITEMIDLIST* parent = CIDAParent(cida);
	m_items.resize(count);
	if (!folder && count > 0)
		ILFolder(&folder, parent);
	for (size_t i = 0; i < count; ++i)
		PathCreate(&m_items[i], folder, parent, CIDAChild(cida, i));
}

XpShellItemArray::XpShellItemArray(const ITEMIDLIST* parent, IShellFolder* folder_, size_t count, ITEMIDLIST** children)
{
	ref<IShellFolder> folder(folder_);
	m_items.resize(count);
	if (!folder && count > 0)
		ILFolder(&folder, parent);
	for (size_t i = 0; i < count; ++i)
		PathCreate(&m_items[i], folder, parent, children[i]);
}

IFACEMETHODIMP XpShellItemArray::BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **pp)
{
//	if (rbhid == BHID_SFUIObject)
//		folder->GetUIObjectOf();
	return E_NOTIMPL;
}

IFACEMETHODIMP XpShellItemArray::GetPropertyStore(GETPROPERTYSTOREFLAGS flags, REFIID riid, void **pp)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpShellItemArray::GetPropertyDescriptionList(REFPROPERTYKEY keyType, REFIID riid, void **pp)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpShellItemArray::GetAttributes(SIATTRIBFLAGS dwAttribFlags, SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP XpShellItemArray::GetItemAt(DWORD index, IShellItem **pp)
{
	if (index >= m_items.size())
		return E_FAIL;
	return m_items[index].copyto(pp);
}

IFACEMETHODIMP XpShellItemArray::GetCount(DWORD* outCount)
{
	ASSERT(outCount);
	*outCount = (DWORD)m_items.size();
	return S_OK;
}

IFACEMETHODIMP XpShellItemArray::EnumItems(IEnumShellItems** pp)
{
	return E_NOTIMPL;
}

//==========================================================================================================================

HRESULT XpCreateShellItemArray(IShellItemArray** pp, const CIDA* cida, IShellFolder* folder)
{
	ASSERT(pp);
	ASSERT(cida);
	*pp = new XpShellItemArray(cida, folder);
	return S_OK;
}

HRESULT XpCreateShellItemArray(IShellItemArray** pp, const ITEMIDLIST* parent, IShellFolder* folder, size_t count, ITEMIDLIST** children)
{
	ASSERT(pp);
	*pp = new XpShellItemArray(parent, folder, count, children);
	return S_OK;
}
