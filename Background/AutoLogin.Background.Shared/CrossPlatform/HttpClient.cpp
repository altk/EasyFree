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
class AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::HttpClientImpl final
{
public:
	HttpClientImpl()
	{
		ComPtr<IHttpClientFactory> httpClientFactory;
		ComPtr<IHttpFilter> httpFilter;
		ComPtr<IHttpBaseProtocolFilter> httpBaseProtocolFilter;
		ComPtr<IHttpCacheControl> httpCacheControl;
		ComPtr<IVector<ChainValidationResult>> validationResultVector;
		ComPtr<IHttpMethodStatics> httpMethodStatics;

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

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpRequestMessage).Get(),
								   &_httpRequestMessageFactory));

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpMethod).Get(),
								   &httpMethodStatics));

		Check(httpMethodStatics->get_Get(&_httpGetMethod));

		Check(httpMethodStatics->get_Post(&_httpPostMethod));
	}

	task<ComPtr<IHttpResponseMessage>> GetAsync(wstring url,
												vector<tuple<wstring, wstring>> headers) const NOEXCEPT
	{
		ComPtr<IHttpRequestMessage> httpRequestMessage;
		ComPtr<IHttpRequestHeaderCollection> httpRequestHeaderCollection;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;

		Check(_httpRequestMessageFactory->Create(_httpGetMethod.Get(),
												 CreateUri(move(url)).Get(),
												 &httpRequestMessage));

		Check(httpRequestMessage->get_Headers(&httpRequestHeaderCollection));

		for(auto&& header : headers)
		{
			boolean isSuccess;
			HString key(get<0>(header));
			HString value(get<1>(header));
			Check(httpRequestHeaderCollection->TryAppendWithoutValidation(key.Get(),
																		  value.Get(),
																		  &isSuccess));
		}

		Check(_httpClient->SendRequestAsync(httpRequestMessage.Get(),
											&getAsyncOperation));

		return GetTask(getAsyncOperation.Get());
	}

	task<ComPtr<IHttpResponseMessage>> PostAsync(wstring url,
												 vector<tuple<wstring, wstring>> headers,
												 wstring postContent) const NOEXCEPT
	{
		ComPtr<IHttpStringContentFactory> stringContentFactory;
		ComPtr<IHttpContent> postHttpContent;
		ComPtr<IHttpRequestMessage> httpRequestMessage;
		ComPtr<IHttpRequestHeaderCollection> httpRequestHeaderCollection;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> postAsyncOperation;

		Check(_httpRequestMessageFactory->Create(_httpPostMethod.Get(),
												 CreateUri(move(url)).Get(),
												 &httpRequestMessage));

		Check(httpRequestMessage->get_Headers(&httpRequestHeaderCollection));

		for (auto&& header : headers)
		{
			boolean isSuccess;
			HString key(get<0>(header));
			HString value(get<1>(header));
			Check(httpRequestHeaderCollection->TryAppendWithoutValidation(key.Get(),
																		  value.Get(),
																		  &isSuccess));
		}

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpStringContent).Get(),
								   &stringContentFactory));

		Check(stringContentFactory->CreateFromString(HStringReference(postContent.data(), postContent.size()).Get(),
													 &postHttpContent));

		Check(httpRequestMessage->put_Content(postHttpContent.Get()));

		Check(_httpClient->SendRequestAsync(httpRequestMessage.Get(),
											&postAsyncOperation));

		return GetTask(postAsyncOperation.Get());
	}

private:
	ComPtr<IHttpClient> _httpClient;
	ComPtr<IHttpRequestMessageFactory> _httpRequestMessageFactory;
	ComPtr<IHttpMethod> _httpGetMethod;
	ComPtr<IHttpMethod> _httpPostMethod;

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
AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::HttpClient() NOEXCEPT
	: _impl(new HttpClientImpl()) {}

template <>
AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::~HttpClient() NOEXCEPT {}

task<ComPtr<IHttpResponseMessage>> AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::GetAsync(wstring url,
																												vector<tuple<wstring, wstring>> headers) const NOEXCEPT
{
	return _impl->GetAsync(move(url),
						   move(headers));
}

task<ComPtr<IHttpResponseMessage>> AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::PostAsync(wstring url,
																												 vector<tuple<wstring, wstring>> headers,
																												 wstring postContent) const NOEXCEPT
{
	return _impl->PostAsync(move(url),
							move(headers),
							move(postContent));
}
