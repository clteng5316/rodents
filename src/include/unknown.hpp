// unknown.hpp
#pragma once

#include "meta.hpp"
#include "preprocessor.hpp"
#include "ref.hpp"

//========================================================================================================================
// implements.

template
<
	class T0 = meta::Void, class T1 = meta::Void, class T2 = meta::Void, class T3 = meta::Void, class T4 = meta::Void,
	class T5 = meta::Void, class T6 = meta::Void, class T7 = meta::Void, class T8 = meta::Void, class T9 = meta::Void
> 
struct implements;

//========================================================================================================================
// mixin.

template
<
	template < typename > class T0 = meta::Void1, template < typename > class T1 = meta::Void1,
	template < typename > class T2 = meta::Void1, template < typename > class T3 = meta::Void1,
	template < typename > class T4 = meta::Void1, template < typename > class T5 = meta::Void1,
	template < typename > class T6 = meta::Void1, template < typename > class T7 = meta::Void1,
	template < typename > class T8 = meta::Void1, template < typename > class T9 = meta::Void1
> 
struct mixin;

#include "unknown.inl"

//========================================================================================================================
// �������ԊǗ�.

/// �X�^�b�N��ɍ����I�u�W�F�N�g�̐������ԊǗ�.
template < class TBase >
class __declspec(novtable) StaticLife : public TBase
{
public:
	ULONG STDMETHODCALLTYPE AddRef()  throw()	{ return 1; }
	ULONG STDMETHODCALLTYPE Release() throw()	{ return 1; }
};

/// �q�[�v��ɍ����I�u�W�F�N�g�̐������ԊǗ�.
template < class TBase >
class __declspec(novtable) DynamicLife : public TBase
{
protected:
	volatile LONG m_refcount;
	DynamicLife() throw() : m_refcount(1)	{}
	virtual ~DynamicLife() throw()
	{
		ASSERT(m_refcount == 0);
	}
public:
	ULONG STDMETHODCALLTYPE AddRef() throw()
	{
		ASSERT(m_refcount > 0);
		return ::InterlockedIncrement(&m_refcount);
	}
	ULONG STDMETHODCALLTYPE Release() throw()
	{
		ASSERT(m_refcount > 0);
		ULONG refcount = ::InterlockedDecrement(&m_refcount);
		if (refcount == 0)
			delete this;
		return refcount;
	}
};

//========================================================================================================================
// IUnknown�����x�[�X.

template < typename TBase, typename TImplements = implements<>, typename TMixin = mixin<> >
class __declspec(novtable) UnknownDerived : public detail::Unknown1<TBase, TImplements, TMixin>::Result
{
//	typedef meta::Typelist		__inherits__;	// C++����I�Ɍp�����Ă���C���^�t�F�[�X.
//	typedef meta::Typelist		__supports__;	// COM�I�u�W�F�N�g�Ƃ��ăT�|�[�g����C���^�t�F�[�X.
//	typedef ...					__primary__;	// �ł��u�����v�̃C���^�t�F�[�X.

//	IUnknown*	OID;	// COM�A�C�f���e�B�e�B�Ƃ��Ă�IUnknown�|�C���^.

//	HRESULT	STDMETHODCALLTYPE	QueryInterface(REFIID iid, void** pp) throw();
//	HRESULT	STDMETHODCALLTYPE	QueryInterface(REFINTF pp) throw();
//	ULONG	STDMETHODCALLTYPE	AddRef() throw();
//	ULONG	STDMETHODCALLTYPE	Release() throw();
};

template < typename TImplements, typename TMixin = mixin<DynamicLife> >
class __declspec(novtable) Unknown : public detail::Unknown1<Void, TImplements, TMixin>::Result
{
};

template < typename T >
inline HRESULT newobj(REFIID iid, void** pp)
{
	ref<IUnknown> p;
	p.attach((new T())->OID);
	return p.copyto(iid, pp);
}

template < typename T >
inline HRESULT newobj(REFINTF pp)
{
	return newobj<T>(pp.iid, pp.pp);
}
