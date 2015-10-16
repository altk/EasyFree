#pragma once
#include <AutoLogin.Background_h.h>
#include <macro.h>
#include <MTL\Implements\ActivationFactory.h>
#include <MTL\Implements\RuntimeClass.h>

namespace AutoLogin
{
	namespace Background
	{
		namespace Implementations
		{
			class LoginTask final : public MTL::Implements::RuntimeClass<ABI::Windows::ApplicationModel::Background::IBackgroundTask>
			{
			public:
				STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override;
				STDMETHODIMP Run(ABI::Windows::ApplicationModel::Background::IBackgroundTaskInstance* taskInstance) NOEXCEPT override;
			};

			class LoginTaskFactory final : public MTL::Implements::ActivationFactory<LoginTask>
			{
			public:
				STDMETHODIMP GetRuntimeClassName(HSTRING*) NOEXCEPT override
				{
					return E_ILLEGAL_METHOD_CALL;
				}
			};
		}
	}
}