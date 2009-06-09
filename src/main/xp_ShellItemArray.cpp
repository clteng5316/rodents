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

	class XpEnumShellItems : public Unknown< implements<IEnumShellItems> >
	{
	private:
		ref<IShellItemArray>	m_array;
		DWORD					m_index;

	public:
		XpEnumShellItems(IShellItemArray* array);
        IFACEMETHODIMP Next(ULONG num, IShellItem** items, ULONG* fetched);
        IFACEMETHODIMP Skip(ULONG num);
        IFACEMETHODIMP Reset();
        IFACEMETHODIMP Clone(IEnumShellItems** pp);
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
	if (!pp)
		return E_POINTER;
	*pp = new XpEnumShellItems(this);
	return S_OK;
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

//==========================================================================================================================

XpEnumShellItems::XpEnumShellItems(IShellItemArray* array)
: m_array(array), m_index(0)
{
	ASSERT(m_array);
}

IFACEMETHODIMP XpEnumShellItems::Next(ULONG num, IShellItem** items, ULONG* fetched)
{
	HRESULT		hr;
	ULONG		done;

	if (num < 1)
		return E_INVALIDARG;
	if (!fetched)
		fetched = &done;

	for (*fetched = 0; *fetched < num; ++*fetched, ++m_index)
	{
		if FAILED(hr = m_array->GetItemAt(m_index, &items[*fetched]))
			return *fetched > 0 ? S_FALSE : hr;
	}
	return S_OK;
}

IFACEMETHODIMP XpEnumShellItems::Skip(ULONG num)
{
	m_index += num;
	return S_OK;
}

IFACEMETHODIMP XpEnumShellItems::Reset()
{
	m_index = 0;
	return S_OK;
}

IFACEMETHODIMP XpEnumShellItems::Clone(IEnumShellItems** pp)
{
	if (!pp)
		return E_POINTER;
	*pp = new XpEnumShellItems(m_array);
	return S_OK;
}
