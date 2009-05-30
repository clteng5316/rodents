// unknown.inl
#pragma once

#define PP_VOID(n)		meta::Void
#define PP_VOID1(n)		meta::Void1

//========================================================================================================================
// implements.

template
<
	class T0, class T1, class T2, class T3, class T4,
	class T5, class T6, class T7, class T8, class T9
> 
struct implements
{
	typedef meta::Typelist<T0, typename implements<T1, T2, T3, T4, T5, T6, T7, T8, T9>::Result > Result;
};

template <> struct implements < PP_CSV(10, PP_VOID) >
{
	typedef meta::Void	Result;
};

//========================================================================================================================
// mixin.

template
<
	template < typename > class T0, template < typename > class T1,
	template < typename > class T2, template < typename > class T3,
	template < typename > class T4, template < typename > class T5,
	template < typename > class T6, template < typename > class T7,
	template < typename > class T8, template < typename > class T9
> 
struct mixin
{
	template < typename TBase > struct Result1
	{
		typedef T0< typename mixin<T1, T2, T3, T4, T5, T6, T7, T8, T9>::Result1<TBase>::Result > Result;
	};
};

template <> struct mixin < PP_CSV(10, PP_VOID1) >
{
	template < typename TBase > struct Result1
	{
		typedef TBase Result;
	};
};

//========================================================================================================================

namespace detail
{
	using namespace meta;

	inline HRESULT InterfaceAssign(IUnknown* p, void** pp)
	{
		*pp = p;
		p->AddRef();
		return S_OK;
	}

	//========================================================================================================================

	template < class TInherits, class TSupports >
	class __declspec(novtable) Interface : public Inherits<TInherits>
	{
	public:
		typedef typename TInherits::Head __primary__; ///< OIDに対応するインタフェース.
		IUnknown* get_OID() throw()	{ return static_cast<IUnknown*>(static_cast<__primary__*>(this)); }
		__declspec(property(get=get_OID)) IUnknown* OID;

	public: // IUnknown
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** pp) throw()
		{
			return InternalQueryInterface<TSupports>(iid, pp);
		}
//		ULONG STDMETHODCALLTYPE AddRef() throw() = 0;
//		ULONG STDMETHODCALLTYPE Release() throw() = 0;

	private:
		template < typename Interface > Interface* InternalGetInterface()
		{
			STATIC_ASSERT( SUPERSUBCLASS(IUnknown, Interface) );
			typedef typename MostDerived<TInherits, Interface>::Result Derived;
			return static_cast<Interface*>(static_cast<Derived*>(this));
		}
		template < typename TList >
		HRESULT STDMETHODCALLTYPE InternalQueryInterface(REFIID iid, void** pp) throw()
		{
			if (iid == __uuidof(TList::Head))
				return InterfaceAssign(InternalGetInterface<TList::Head>(), pp);
			else
				return InternalQueryInterface<TList::Tail>(iid, pp);
		}
		template <>
		HRESULT STDMETHODCALLTYPE InternalQueryInterface<Void>(REFIID iid, void** pp) throw()
		{
			if (iid == __uuidof(IUnknown))
				return InterfaceAssign(OID, pp);
			else
			{
				*pp = null;
				return E_NOINTERFACE;
			}
		}
	};

	//========================================================================================================================

	template < class TBase, class TSupports, typename TMixin >
	class Unknown2
	{
		typedef typename MostDerivedOnly<TSupports>::Result	TInherits;
		typedef typename TMixin::Result1< Interface<TInherits, TSupports> >::Result	Mixin;
	public:
		class __declspec(novtable) Result : public TBase, public Mixin
		{
			typedef TBase	TLeft;
			typedef Mixin	TRight;
		public:
			typedef Union<typename TLeft::__inherits__, TInherits>	__inherits__;
			typedef Union<typename TLeft::__supports__, TSupports>	__supports__;

			// IUnknownの実装はすべてTLeftへ転送する。
			typedef typename TLeft::__primary__	__primary__;
			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** pp) throw()
			{
				HRESULT hr = TLeft::QueryInterface(iid, pp);
				if (hr != E_NOINTERFACE)
					return hr;
				else
					return TRight::QueryInterface(iid, pp);
			}
			ULONG STDMETHODCALLTYPE AddRef() throw()	{ return TLeft::AddRef(); }
			ULONG STDMETHODCALLTYPE Release() throw()	{ return TLeft::Release(); }
			IUnknown* get_OID() throw()			{ return TLeft::get_OID(); }
			__declspec(property(get=get_OID)) IUnknown* OID;
		};
	};

	template < class TSupports, typename TMixin >
	class Unknown2<Void, TSupports, TMixin>
	{
		typedef typename MostDerivedOnly<TSupports>::Result	TInherits;
		typedef typename TMixin::Result1< Interface<TInherits, TSupports> >::Result	Mixin;
	public:
		// これがルートクラス.
		class __declspec(novtable) Result : public Mixin
		{
		public:
			typedef TInherits	__inherits__;
			typedef TSupports	__supports__;
			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** pp) throw()
			{
				if (!pp)
					return E_INVALIDARG;
				return __super::QueryInterface(iid, pp);
			}
			HRESULT STDMETHODCALLTYPE QueryInterface(REFINTF pp) throw()
			{
				return QueryInterface(pp.iid, pp.pp);
			}
		};
	};

	template < class TBase, typename TMixin >
	class Unknown2<TBase, Void, TMixin>
	{
	public:
		typedef typename TMixin::Result1<TBase>::Result Result;
	};

	template < typename TMixin >
	class Unknown2<Void, Void, TMixin>
	{
	public:
		typedef typename TMixin::Result1<Void>::Result Result;
	};

	//========================================================================================================================

	template < class TBase, typename TImplements, typename TMixin >
	class Unknown1
	{
		typedef typename meta::DerivedToFront<typename TImplements::Result>::Result	Sorted;
		typedef typename Subtract<Sorted, typename TBase::__supports__>::Result		Avail;
	public:
		typedef typename Unknown2<TBase, Avail, TMixin>::Result Result;
	};

	template < class TBase, typename TMixin >
	class Unknown1<TBase, implements<>, TMixin>
	{
	public:
		typedef typename Unknown2<TBase, Void, TMixin>::Result Result;
	};

	template < typename TImplements, typename TMixin >
	class Unknown1<Void, TImplements, TMixin>
	{
		typedef typename meta::DerivedToFront<typename TImplements::Result>::Result	Avail;
	public:
		typedef typename Unknown2<Void, Avail, TMixin>::Result Result;
	};

	template < typename TMixin >
	class Unknown1<Void, implements<>, TMixin>
	{
	public:
		typedef typename Unknown2<Void, Void, TMixin>::Result Result;
	};
}

#undef PP_VOID
