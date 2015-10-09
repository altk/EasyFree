#pragma once
#include <utility>
#include <Unknwnbase.h>
#include <macro.h>

namespace MTL
{
	namespace Internals
	{
		template <typename TInterface>
		struct NOVTABLE RemoveIUnknown abstract : public TInterface
		{
			static_assert(std::is_base_of<IUnknown, TInterface>::value, "TInterface must inherit IUnknown");

		private:

			STDMETHODIMP_(ULONG) AddRef() ;
			STDMETHODIMP_(ULONG) Release();
			STDMETHODIMP QueryTInterface(IID, void** ppvObject);
		};
	}
}
