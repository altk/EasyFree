#include "pch.h"
#include "MosMetroAuthorizer.h"
#include <robuffer.h>
#include <windows.web.http.h>
#include <windows.storage.streams.h>
#include <windows.networking.connectivity.h>
#include <MTL.h>
#include <MosMetroResponseParcer.h>

using namespace std;
using namespace Concurrency;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Web::Http::Headers;
using namespace ABI::Windows::Web::Http::Filters;
using namespace ABI::Windows::Web::Http;
using namespace ABI::Windows::Storage::Streams;
using namespace Windows::Storage::Streams;
using namespace MTL;
using namespace AutoLogin::CrossPlatform;
using namespace AutoLogin::CrossPlatform;

class MosMetroAuthorizerImpl final
{
public:
	static string GetAuthUrl() NOEXCEPT
	{
		return "https://login.wi-fi.ru/am/UI/Login";
	}

	static task<AuthStatus::Enum> Authorize() NOEXCEPT
	{
		try
		{
			static wstring bindUrl = L"http://httpbin.org/status/200";

			auto httpClient = CreateHttpClient(false);

			return GetAsync(httpClient.Get(), bindUrl)
					.then([](IHttpResponseMessage* response)
						{
							return GetContentAsync(response);
						})
					.then([](IBuffer* buffer) -> string
						{
							ComPtr<IBufferByteAccess> contentBytes;
							Check(buffer->QueryInterface<IBufferByteAccess>(&contentBytes));

							byte* content;
							Check(contentBytes->Buffer(&content));

							return MosMetroResponseParser::GetFormUrl(reinterpret_cast<const char*>(content));
						})
					.then([httpClient](string url) -> task<AuthStatus::Enum>
						{
							//Запрос прошёл без каких-либо проблем, поэтому URL для перехода не найден
							if (url.empty())
							{
								return task_from_result(AuthStatus::None);
							}

							//Проверяем на полное совпадение, если успешно, значит устройство ещё не зарегистрировано
							if (url == GetAuthUrl())
							{
								return task_from_result(AuthStatus::Unauthorized);
							}

							return GetAsync(httpClient.Get(), wstring(begin(url), end(url)))
									.then([httpClient](IHttpResponseMessage* response) -> task<IHttpResponseMessage*>
										{
											ComPtr<IHttpRequestMessage> request;
											Check(response->get_RequestMessage(&request));

											ComPtr<IUriRuntimeClass> locationUri;
											Check(request->get_RequestUri(&locationUri));

											HString absoluteUri;
											Check(locationUri->get_AbsoluteUri(&absoluteUri));

											return GetContentAsync(response).then([](IBuffer* buffer)
																				{
																					return GetPostContent(buffer);
																				})
																			.then([httpClient, locationUri](wstring postContent)
																				{
																					return AuthAsync(httpClient.Get(),
																									 locationUri.Get(),
																									 move(postContent));
																				});
										})
									.then([httpClient](IHttpResponseMessage* authResponse)
										{
											return GetAsync(httpClient.Get(), bindUrl);
										})
									.then([](IHttpResponseMessage* checkResponse) -> AuthStatus::Enum
										{
											boolean isSuccessStatusCode;
											Check(checkResponse->get_IsSuccessStatusCode(&isSuccessStatusCode));

											if (isSuccessStatusCode > 0)
											{
												return AuthStatus::Success;
											}
											else
											{
												return AuthStatus::Fail;
											}
										});
						})
					.then([](task<AuthStatus::Enum> result) NOEXCEPT -> AuthStatus::Enum
						{
							try
							{
								return result.get();
							}
							catch (...) {}
							return AuthStatus::None;
						});
		}
		catch (...) {}
		return task_from_result(AuthStatus::None);
	}

	static bool CanAuth(const wchar_t* const connectionName) NOEXCEPT
	{
		if (nullptr == connectionName) return false;

		return wcscmp(connectionName, L"MosMetro_Free") == 0;
	}

private:
	static wstring GetPostContent(IBuffer* responseContent)
	{
		auto buffer = CreateComPtr(responseContent);

		ComPtr<IBufferByteAccess> bufferByteAccess;
		Check(buffer.As(&bufferByteAccess));

		byte* content;
		Check(bufferByteAccess->Buffer(&content));

		UINT32 lenght;
		Check(buffer->get_Length(&lenght));

		auto postContent = MosMetroResponseParser::GetPostString(reinterpret_cast<const char*>(content));
		return wstring(begin(postContent), end(postContent));
	}

	static ComPtr<IHttpClient> CreateHttpClient(bool allowAutoRedirect)
	{
		ComPtr<IHttpClientFactory> httpClientFactory;
		ComPtr<IHttpFilter> httpFilter;
		ComPtr<IHttpBaseProtocolFilter> httpBaseProtocolFilter;
		ComPtr<IHttpClient> httpClient;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpClient).Get(),
								   &httpClientFactory));

		Check(ActivateInstance<IHttpBaseProtocolFilter>(HStringReference(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter).Get(),
														&httpBaseProtocolFilter));

		Check(httpBaseProtocolFilter->put_AllowAutoRedirect(allowAutoRedirect));

		Check(httpBaseProtocolFilter.As(&httpFilter));

		Check(httpClientFactory->Create(httpFilter.Get(),
										&httpClient));

		return httpClient;
	}

	static task<IHttpResponseMessage*> GetAsync(IHttpClient* httpClient, wstring url)
	{
		ComPtr<IUriRuntimeClassFactory> uriFactory;
		ComPtr<IUriRuntimeClass> uri;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
								   &uriFactory));

		Check(uriFactory->CreateUri(HStringReference(url.data(), url.size()).Get(),
									&uri));

		Check(httpClient->GetAsync(uri.Get(),
								   &getAsyncOperation));

		return GetTask(getAsyncOperation.Get());
	}

	static task<IBuffer*> GetContentAsync(IHttpResponseMessage* response)
	{
		ComPtr<IHttpContent> httpContent;
		ComPtr<IAsyncOperationWithProgress<IBuffer*, ULONGLONG>> readAsBufferOperation;

		Check(response->get_Content(&httpContent));

		Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

		return GetTask(readAsBufferOperation.Get());
	}

	static task<IHttpResponseMessage*> AuthAsync(IHttpClient* httpClient,
												 IUriRuntimeClass* uri,
												 wstring postContent)
	{
		ComPtr<IHttpStringContentFactory> stringContentFactory;
		GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpStringContent).Get(),
							 &stringContentFactory);

		ComPtr<IHttpContent> postHttpContent;
		stringContentFactory->CreateFromString(HStringReference(postContent.data(), postContent.size()).Get(),
											   &postHttpContent);

		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> postAsyncOperation;
		httpClient->PostAsync(uri,
							  postHttpContent.Get(),
							  &postAsyncOperation);

		return GetTask(postAsyncOperation.Get());
	}
};

task<AuthStatus::Enum> MosMetroAuthorizer::AuthAsync() const NOEXCEPT
{
	return MosMetroAuthorizerImpl::Authorize();
}

bool MosMetroAuthorizer::CanAuth(const wchar_t* const connectionName) const NOEXCEPT
{
	return MosMetroAuthorizerImpl::CanAuth(connectionName);
}

string MosMetroAuthorizer::GetAuthUrl() const NOEXCEPT
{
	return MosMetroAuthorizerImpl::GetAuthUrl();
}