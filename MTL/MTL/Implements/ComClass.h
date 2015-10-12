#pragma once
#include <Unknwn.h>
#include <macro.h>
#include <MTL\Internals\utility.h>

namespace MTL
{
	namespace Implements
	{
		template <typename T, typename ... Ts>
		class
				NOVTABLE ComClass : public T, public Ts...
		{
			static_assert(Internals::variadic_is_base_of<IUnknown, T, Ts...>::value, "Not all interfaces inherit IUnknown.");
			static_assert(Internals::is_type_set<T, Ts...>::value, "Found duplicate types. You must specify unique types.");

		public:
			STDMETHODIMP_(ULONG) AddRef() NOEXCEPT override final
			{
				return InterlockedIncrement(&_counter);
			}

			STDMETHODIMP_(ULONG) Release() NOEXCEPT override final
			{
				auto const remaining = InterlockedDecrement(&_counter);
				if (0 == remaining)
				{
					delete this;
				}
				return remaining;
			}

			STDMETHODIMP QueryInterface(const GUID& guid, void** result) NOEXCEPT override final
			{
				if (guid == __uuidof(T) ||
					guid == __uuidof(IUnknown) ||
					guid == __uuidof(IInspectable))
				{
					*result = this;
				}
				else
				{
					*result = QueryInterfaceImpl<Ts...>(guid);
				}

				if (nullptr == *result) return E_NOINTERFACE;

				static_cast<IUnknown*>(*result)->AddRef();
				return S_OK;
			}

		protected:
			template <typename U, typename ... Us>
			void* QueryInterfaceImpl(const GUID& guid) NOEXCEPT
			{
				using namespace Internals;

				if (is_cloaked<U>::value || guid != __uuidof(U))
				{
					return QueryInterfaceImpl<Us...>(guid);
				}
				return static_cast<U*>(this);
			}

			template <int = 0>
			void* QueryInterfaceImpl(const GUID&) const NOEXCEPT
			{
				return nullptr;
			}

		private:
			volatile ULONG _counter = 1;
		};
	}
}
