#pragma once
#include <activation.h>
#include "RuntimeClass.h"

namespace MTL
{
	namespace Implements
	{
		template<typename T>
		class ActivationFactory : public RuntimeClass<IActivationFactory>
		{
		public:
			STDMETHODIMP ActivateInstance(IInspectable** instance) NOEXCEPT override final
			{
				*instance = new (std::nothrow) T();
				if(nullptr == *instance)
				{
					return E_OUTOFMEMORY;
				}
				return S_OK;
			}
		};
	}
}