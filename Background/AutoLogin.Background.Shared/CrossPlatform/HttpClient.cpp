#include <pch.h>
#include <HttpClient.h>
#include <windows.web.http.h>
#include <windows.foundation.collections.h>

using namespace std;
using namespace Concurrency;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Web::Http::Headers;
using namespace ABI::Windows::Web::Http::Filters;
using namespace ABI::Windows::Web::Http;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Security::Cryptography::Certificates;
using namespace MTL;
using namespace AutoLogin::CrossPlatform;

template <>
class AutoLogin::CrossPlatform::HttpClient<IHttpResponseMessage*>::HttpClientImpl final
{
public:
	HttpClientImpl()
	{
		ComPtr<IHttpClientFactory> httpClientFactory;
		ComPtr<IHttpFilter> httpFilter;
		ComPtr<IHttpBaseProtocolFilter> httpBaseProtocolFilter;
		ComPtr<IHttpCacheControl> httpCacheControl;
		ComPtr<IVector<ChainValidationResult>> validationResultVector;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpClient).Get(),
								   &httpClientFactory));

		Check(ActivateInstance<IHttpBaseProtocolFilter>(HStringReference(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter).Get(),
														&httpBaseProtocolFilter));

		Check(httpBaseProtocolFilter->get_CacheControl(&httpCacheControl));

		Check(httpBaseProtocolFilter->put_AllowUI(false));

		Check(httpBaseProtocolFilter->get_IgnorableServerCertificateErrors(&validationResultVector));

		Check(validationResultVector->Append(ChainValidationResult_Expired));
		
		Check(validationResultVector->Append(ChainValidationResult_Untrusted));
		
		Check(validationResultVector->Append(ChainValidationResult_InvalidName));

		Check(httpCacheControl->put_ReadBehavior(HttpCacheReadBehavior_MostRecent));

		Check(httpCacheControl->put_WriteBehavior(HttpCacheWriteBehavior_NoCache));

		Check(httpBaseProtocolFilter->put_AllowAutoRedirect(true));

		Check(httpBaseProtocolFilter.As(&httpFilter));

		Check(httpClientFactory->Create(httpFilter.Get(),
										&_httpClient));
	}

	task<IHttpResponseMessage*> GetAsync(wstring url) const NOEXCEPT
	{
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;
		Check(_httpClient->GetAsync(CreateUri(move(url)).Get(),
									&getAsyncOperation));

		return GetTask(getAsyncOperation.Get());
	}

	task<IHttpResponseMessage*> PostAsync(wstring url,
										  wstring postContent) const NOEXCEPT
	{
		ComPtr<IHttpStringContentFactory> stringContentFactory;
		ComPtr<IHttpContent> postHttpContent;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> postAsyncOperation;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpStringContent).Get(),
								   &stringContentFactory));

		Check(stringContentFactory->CreateFromString(HStringReference(postContent.data(), postContent.size()).Get(),
													 &postHttpContent));

		Check(_httpClient->PostAsync(CreateUri(url).Get(),
									 postHttpContent.Get(),
									 &postAsyncOperation));

		return GetTask(postAsyncOperation.Get());
	}

private:
	ComPtr<IHttpClient> _httpClient;

	static ComPtr<IUriRuntimeClass> CreateUri(wstring url)
	{
		ComPtr<IUriRuntimeClassFactory> uriFactory;
		ComPtr<IUriRuntimeClass> uri;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
								   &uriFactory));

		Check(uriFactory->CreateUri(HStringReference(url.data(), url.size()).Get(),
									&uri));

		return uri;
	}
};

template <>
AutoLogin::CrossPlatform::HttpClient<IHttpResponseMessage*>::HttpClient() NOEXCEPT
	: _impl(new HttpClientImpl()) {}

template <>
AutoLogin::CrossPlatform::HttpClient<IHttpResponseMessage*>::~HttpClient() NOEXCEPT {}

task<IHttpResponseMessage*> AutoLogin::CrossPlatform::HttpClient<IHttpResponseMessage*>::GetAsync(wstring url) const NOEXCEPT
{
	return _impl->GetAsync(move(url));
}

task<IHttpResponseMessage*> AutoLogin::CrossPlatform::HttpClient<IHttpResponseMessage*>::PostAsync(wstring url, 
																								   wstring postContent) const NOEXCEPT
{
	return _impl->PostAsync(move(url),
							move(postContent));
}
