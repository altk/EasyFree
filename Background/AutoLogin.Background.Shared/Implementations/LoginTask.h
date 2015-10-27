#pragma once
#include <AutoLogin.Background_h.h>
#include <windows.web.http.h>
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
				static MTL::ComPtr<ABI::Windows::Networking::Connectivity::IConnectionProfile> GetNetworkConnectionProfile() NOEXCEPT;
				static bool CheckProfile(ABI::Windows::Networking::Connectivity::IConnectionProfile* connectionProfile) NOEXCEPT;
				static MTL::ComPtr<ABI::Windows::Web::Http::IHttpClient> CreateHttpClient();
				static void PromtSuccessNotification();
				static void PromtFailNotification();
				static Concurrency::task<ABI::Windows::Web::Http::IHttpResponseMessage*> GetInitialResponseTask(ABI::Windows::Web::Http::IHttpClient* httpClient);
				static Concurrency::task<ABI::Windows::Storage::Streams::IBuffer*> GetResponseContentTask(ABI::Windows::Web::Http::IHttpResponseMessage* response);
				static MTL::ComPtr<ABI::Windows::Foundation::IUriRuntimeClass> GetResponseLocationHeader(ABI::Windows::Web::Http::IHttpResponseMessage* response);
				static std::wstring GetPostContent(ABI::Windows::Storage::Streams::IBuffer* responseContent);
				static Concurrency::task<ABI::Windows::Web::Http::IHttpResponseMessage*> GetAuthResponseTask(ABI::Windows::Web::Http::IHttpClient* httpClient, ABI::Windows::Foundation::IUriRuntimeClass* uri, std::wstring postContent);
			};
		}
	}
}
