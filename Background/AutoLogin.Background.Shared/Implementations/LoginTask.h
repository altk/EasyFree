#pragma once
#include <AutoLogin.Background_h.h>
#include <macro.h>
#include <MTL\Implements\RuntimeClass.h>

namespace AutoLogin
{
	namespace Background
	{
		namespace Implementations
		{
			class LoginTask final : MTL::Implements::RuntimeClass<ABI::Windows::ApplicationModel::Background::IBackgroundTask>
			{
			public:
				STDMETHODIMP GetRuntimeClassName(HSTRING* className) override;
				STDMETHODIMP Run(ABI::Windows::ApplicationModel::Background::IBackgroundTaskInstance* taskInstance) override;
			};
		}
	}
}