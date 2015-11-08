#pragma once
#include <EasyFree.Background_h.h>
#include <macro.h>
#include <MTL.h>

namespace EasyFree
{
	namespace Background
	{
		namespace Implementations
		{
			class LoginTask final : public MTL::RuntimeClass<ABI::Windows::ApplicationModel::Background::IBackgroundTask>
			{
			public:
				STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override;
				STDMETHODIMP Run(ABI::Windows::ApplicationModel::Background::IBackgroundTaskInstance* taskInstance) NOEXCEPT override;			
			};
		}
	}
}
