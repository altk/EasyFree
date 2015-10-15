#pragma once
#include <inspectable.h>
#include <macro.h>
#include <MTL\Implements\ComClass.h>
#include <MTL\Internals\utility.h>

namespace MTL
{
	namespace Implements
	{
		template <typename T, typename ... Ts>
		class RuntimeClass abstract : public ComClass<T, Ts...>
		{
		public:
			STDMETHODIMP GetIids(ULONG* count, GUID** array) NOEXCEPT override final
			{
				using namespace Internals;

				*count = interface_counter<T, Ts...>::typesCount;
				*array = const_cast<GUID*>(iids_extractor<T, Ts...>::getIids());
				if (nullptr == *array)
				{
					return E_OUTOFMEMORY;
				}
				return S_OK;
			}

			STDMETHODIMP GetTrustLevel(TrustLevel* trustLevel) NOEXCEPT override final
			{
				*trustLevel = BaseTrust;
				return S_OK;
			}
		};
	}
}
