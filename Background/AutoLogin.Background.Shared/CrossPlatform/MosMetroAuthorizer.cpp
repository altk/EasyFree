#include "pch.h"
#include "MosMetroAuthorizer.h"
#include <robuffer.h>
#include <windows.web.http.h>
#include <windows.storage.streams.h>
#include <windows.networking.connectivity.h>
#include <MTL.h>

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

template <>
task<bool> MosMetroAuthorizer<IHttpResponseMessage*>::CheckAsync(IHttpResponseMessage* response) NOEXCEPT
{
	boolean isSuccessStatusCode;
	Check(response->get_IsSuccessStatusCode(&isSuccessStatusCode));

	return task_from_result(isSuccessStatusCode == 0);
}

template <>
task<string> MosMetroAuthorizer<IHttpResponseMessage*>::GetContentAsync(IHttpResponseMessage* response) NOEXCEPT
{
	ComPtr<IHttpContent> httpContent;
	ComPtr<IAsyncOperationWithProgress<IBuffer*, ULONGLONG>> readAsBufferOperation;

	Check(response->get_Content(&httpContent));

	Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

	return GetTask(readAsBufferOperation.Get()).then([](IBuffer* buffer) -> string
		{
			if (nullptr == buffer)
			{
				return string();
			}

			ComPtr<IBufferByteAccess> bufferByteAccess;
			Check(buffer->QueryInterface<IBufferByteAccess>(&bufferByteAccess));

			byte* content;
			Check(bufferByteAccess->Buffer(&content));

			UINT32 lenght;
			Check(buffer->get_Length(&lenght));

			return string(content, content + lenght);
		});
}

//class MosMetroAuthorizerImpl final
//{
//public:
//
//	static task<AuthStatus::Enum> Authorize() NOEXCEPT
//	{
//		try
//		{
//			static wstring bindUrl = L"http://httpbin.org/status/200";
//
//			auto httpClient = CreateHttpClient(false);
//
//			return GetAsync(httpClient.Get(), bindUrl)
//					.then([](IHttpResponseMessage* response)
//						{
//							return GetContentAsync(response);
//						})
//					.then([](IBuffer* buffer) -> string
//						{
//							ComPtr<IBufferByteAccess> contentBytes;
//							Check(buffer->QueryInterface<IBufferByteAccess>(&contentBytes));
//
//							byte* content;
//							Check(contentBytes->Buffer(&content));
//
//							return MosMetroResponseParser::GetFormUrl(reinterpret_cast<const char*>(content));
//						})
//					.then([httpClient](string url) -> task<AuthStatus::Enum>
//						{
//							//Запрос прошёл без каких-либо проблем, поэтому URL для перехода не найден
//							if (url.empty())
//							{
//								return task_from_result(AuthStatus::None);
//							}
//
//							//Проверяем на полное совпадение, если успешно, значит устройство ещё не зарегистрировано
//							if (url == GetAuthUrl())
//							{
//								return task_from_result(AuthStatus::Unauthorized);
//							}
//
//							return GetAsync(httpClient.Get(), wstring(begin(url), end(url)))
//									.then([httpClient](IHttpResponseMessage* response) -> task<IHttpResponseMessage*>
//										{
//											ComPtr<IHttpRequestMessage> request;
//											Check(response->get_RequestMessage(&request));
//
//											ComPtr<IUriRuntimeClass> locationUri;
//											Check(request->get_RequestUri(&locationUri));
//
//											HString absoluteUri;
//											Check(locationUri->get_AbsoluteUri(&absoluteUri));
//
//											return GetContentAsync(response).then([](IBuffer* buffer)
//																				{
//																					return GetPostContent(buffer);
//																				})
//																			.then([httpClient, locationUri](wstring postContent)
//																				{
//																					return PostAsync(httpClient.Get(),
//																									 locationUri.Get(),
//																									 move(postContent));
//																				});
//										})
//									.then([httpClient](IHttpResponseMessage* authResponse)
//										{
//											return GetAsync(httpClient.Get(), bindUrl);
//										})
//									.then([](IHttpResponseMessage* checkResponse) -> AuthStatus::Enum
//										{
//											boolean isSuccessStatusCode;
//											Check(checkResponse->get_IsSuccessStatusCode(&isSuccessStatusCode));
//
//											if (isSuccessStatusCode > 0)
//											{
//												return AuthStatus::Success;
//											}
//											else
//											{
//												return AuthStatus::Fail;
//											}
//										});
//						})
//					.then([](task<AuthStatus::Enum> result) NOEXCEPT -> AuthStatus::Enum
//						{
//							try
//							{
//								return result.get();
//							}
//							catch (...) {}
//							return AuthStatus::None;
//						});
//		}
//		catch (...) {}
//		return task_from_result(AuthStatus::None);
//	}
//
//	
//
//private:
//	static wstring GetPostContent(IBuffer* responseContent)
//	{
//		auto buffer = CreateComPtr(responseContent);
//
//		ComPtr<IBufferByteAccess> bufferByteAccess;
//		Check(buffer.As(&bufferByteAccess));
//
//		byte* content;
//		Check(bufferByteAccess->Buffer(&content));
//
//		UINT32 lenght;
//		Check(buffer->get_Length(&lenght));
//
//		auto postContent = MosMetroResponseParser::GetPostString(reinterpret_cast<const char*>(content));
//		return wstring(begin(postContent), end(postContent));
//	}
//
//	static ComPtr<IHttpClient> CreateHttpClient(bool allowAutoRedirect)
//	{
//		ComPtr<IHttpClientFactory> httpClientFactory;
//		ComPtr<IHttpFilter> httpFilter;
//		ComPtr<IHttpBaseProtocolFilter> httpBaseProtocolFilter;
//		ComPtr<IHttpClient> httpClient;
//
//		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpClient).Get(),
//								   &httpClientFactory));
//
//		Check(ActivateInstance<IHttpBaseProtocolFilter>(HStringReference(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter).Get(),
//														&httpBaseProtocolFilter));
//
//		Check(httpBaseProtocolFilter->put_AllowAutoRedirect(allowAutoRedirect));
//
//		Check(httpBaseProtocolFilter.As(&httpFilter));
//
//		Check(httpClientFactory->Create(httpFilter.Get(),
//										&httpClient));
//
//		return httpClient;
//	}
//
//	static ComPtr<IUriRuntimeClass> CreateUri(wstring url)
//	{
//		ComPtr<IUriRuntimeClassFactory> uriFactory;
//		ComPtr<IUriRuntimeClass> uri;
//
//		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
//								   &uriFactory));
//
//		Check(uriFactory->CreateUri(HStringReference(url.data(), url.size()).Get(),
//									&uri));
//
//		return uri;
//	}
//
//	static task<IBuffer*> GetContentAsync(IHttpResponseMessage* response)
//	{
//		ComPtr<IHttpContent> httpContent;
//		ComPtr<IAsyncOperationWithProgress<IBuffer*, ULONGLONG>> readAsBufferOperation;
//
//		Check(response->get_Content(&httpContent));
//
//		Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));
//
//		return GetTask(readAsBufferOperation.Get());
//	}
//
//	static void GetAsync(ComPtr<IHttpClient> httpClient,
//						 ComPtr<IUriRuntimeClass> uri,
//						 task_completion_event<IHttpResponseMessage*> tce,
//						 uint_fast16_t attempt = 0)
//	{
//		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;
//		Check(httpClient->GetAsync(uri.Get(),
//								   &getAsyncOperation));
//
//		GetTask(getAsyncOperation.Get()).then([httpClient, uri, tce, attempt](task<IHttpResponseMessage*> responseTask)
//			{
//				try
//				{
//					tce.set(responseTask.get());
//				}
//				catch (const exception& ex)
//				{
//					if (attempt < 3)
//					{
//						GetAsync(httpClient, uri, tce, attempt + 1);
//					}
//					else
//					{
//						tce.set_exception(ex);
//					}
//				}
//			});
//	}
//
//	static task<IHttpResponseMessage*> GetAsync(ComPtr<IHttpClient> httpClient,
//												ComPtr<IUriRuntimeClass> uri)
//	{
//		task_completion_event<IHttpResponseMessage*> tce;
//
//		GetAsync(httpClient, uri, tce);
//
//		return task<IHttpResponseMessage*>(tce);
//	}
//
//	static void PostAsync(ComPtr<IHttpClient> httpClient, 
//						  ComPtr<IUriRuntimeClass> uri,
//						  ComPtr<IHttpContent> postContent,
//						  task_completion_event<IHttpResponseMessage*> tce,
//						  uint_fast16_t attempt = 0)
//	{
//		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> postAsyncOperation;
//		
//		Check(httpClient->PostAsync(uri.Get(),
//									postContent.Get(),
//									&postAsyncOperation));
//
//		GetTask(postAsyncOperation.Get()).then([httpClient, uri, postContent, tce, attempt](task<IHttpResponseMessage*> responseTask)
//		{
//			try
//			{
//				tce.set(responseTask.get());
//			}
//			catch (const exception& ex)
//			{
//				if(attempt<3)
//				{
//					PostAsync(httpClient, uri, postContent, tce, attempt + 1);
//				}
//				else
//				{
//					tce.set_exception(ex);
//				}
//				else
//				{
//				}
//			}
//		});
//	}
//
//	static task<IHttpResponseMessage*> PostAsync(ComPtr<IHttpClient> httpClient,
//												 ComPtr<IUriRuntimeClass> uri,
//												 HString postContent)
//	{
//		ComPtr<IHttpStringContentFactory> stringContentFactory;
//		ComPtr<IHttpContent> postHttpContent;
//
//		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpStringContent).Get(),
//								   &stringContentFactory));
//
//		Check(stringContentFactory->CreateFromString(postContent.Get(), &postHttpContent));
//
//		task_completion_event<IHttpResponseMessage*> tce;
//
//		PostAsync(httpClient, uri, postHttpContent, tce);
//
//		return task<IHttpResponseMessage*>(tce);
//	}
//};
//
//task<AuthStatus::Enum> MosMetroAuthorizer::AuthAsync() const NOEXCEPT
//{
//	return MosMetroAuthorizerImpl::Authorize();
//}

