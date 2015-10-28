#pragma once
#include <AutoLogin.Background_h.h>
#include <macro.h>
#include <MTL.h>

namespace AutoLogin
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
			private:
				static void PromtSuccessNotification() NOEXCEPT;
				static void PromtFailNotification() NOEXCEPT;
			};
		}
	}
}
