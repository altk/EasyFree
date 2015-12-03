#include <pch.h>
#include <HttpClient.h>
#include <windows.web.http.h>
#include <windows.foundation.collections.h>
#include <HttpRequestHeaders.h>
#include <MTL.h>

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
	explicit HttpClientImpl(uint_fast16_t maxRetryCount = 3)
		: _maxRetryCount(maxRetryCount)
	{
		ComPtr<IHttpClientFactory> httpClientFactory;
		ComPtr<IHttpFilter> httpFilter;
		ComPtr<IHttpBaseProtocolFilter> httpBaseProtocolFilter;
		ComPtr<IHttpCacheControl> httpCacheControl;
		ComPtr<IVector<ChainValidationResult>> validationResultVector;
		ComPtr<IHttpMethodStatics> httpMethodStatics;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
								   &uriFactory));

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpClient).Get(),
								   &httpClientFactory));

		Check(ActivateInstance<IHttpBaseProtocolFilter>(HStringReference(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter).Get(),
														&httpBaseProtocolFilter));

		Check(httpBaseProtocolFilter->put_AllowUI(false));

		Check(httpBaseProtocolFilter->put_AllowAutoRedirect(false));

		Check(httpBaseProtocolFilter->get_IgnorableServerCertificateErrors(&validationResultVector));

		Check(validationResultVector->Append(ChainValidationResult_Expired));

		Check(validationResultVector->Append(ChainValidationResult_Untrusted));

		Check(validationResultVector->Append(ChainValidationResult_InvalidName));

		Check(httpBaseProtocolFilter->get_CacheControl(&httpCacheControl));

		Check(httpCacheControl->put_ReadBehavior(HttpCacheReadBehavior_MostRecent));

		Check(httpCacheControl->put_WriteBehavior(HttpCacheWriteBehavior_NoCache));

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
												unordered_map<wstring, wstring> headers) const
	{
		task_completion_event<ComPtr<IHttpResponseMessage>> taskCompletionEvent;

		GetAsync(make_shared<wstring>(move(url)),
				 make_shared<unordered_map<wstring, wstring>>(move(headers)),
				 taskCompletionEvent,
				 1);

		return task<ComPtr<IHttpResponseMessage>>(taskCompletionEvent);
	}

	task<ComPtr<IHttpResponseMessage>> PostAsync(wstring url,
												 wstring postContent,
												 unordered_map<wstring, wstring> headers) const
	{
		task_completion_event<ComPtr<IHttpResponseMessage>> taskCompletionEvent;

		PostAsync(make_shared<wstring>(move(url)),
				  make_shared<unordered_map<wstring, wstring>>(move(headers)),
				  make_shared<wstring>(move(postContent)),
				  taskCompletionEvent,
				  1);

		return task<ComPtr<IHttpResponseMessage>>(taskCompletionEvent);
	}

private:
	uint_fast16_t _maxRetryCount;

	ComPtr<IUriRuntimeClassFactory> uriFactory;
	ComPtr<IHttpClient> _httpClient;
	ComPtr<IHttpRequestMessageFactory> _httpRequestMessageFactory;
	ComPtr<IHttpMethod> _httpGetMethod;
	ComPtr<IHttpMethod> _httpPostMethod;

	ComPtr<IUriRuntimeClass> CreateUri(wstring url) const
	{
		ComPtr<IUriRuntimeClass> uri;
		Check(uriFactory->CreateUri(HString(move(url)).Get(),
									&uri));

		return uri;
	}

	ComPtr<IHttpRequestMessage> CreateRequest(IHttpMethod* method,
											  wstring url,
											  unordered_map<wstring, wstring> headers) const
	{
		ComPtr<IHttpRequestMessage> httpRequestMessage;
		ComPtr<IHttpRequestHeaderCollection> httpRequestHeaderCollection;

		Check(_httpRequestMessageFactory->Create(method,
												 CreateUri(url).Get(),
												 &httpRequestMessage));

		Check(httpRequestMessage->get_Headers(&httpRequestHeaderCollection));

		for (auto& header : headers)
		{
			boolean isSuccess;
			Check(httpRequestHeaderCollection->TryAppendWithoutValidation(HString(move(header.first)).Get(),
																		  HString(move(header.second)).Get(),
																		  &isSuccess));
		}

		return httpRequestMessage;
	}

	void GetAsync(shared_ptr<wstring> url,
				  shared_ptr<unordered_map<wstring, wstring>> headers,
				  task_completion_event<ComPtr<IHttpResponseMessage>> taskCompletionEvent,
				  uint_fast16_t tryCount) const
	{
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;

		auto httpRequestMessage = CreateRequest(_httpGetMethod.Get(),
												*url,
												*headers);

		Check(_httpClient->SendRequestAsync(httpRequestMessage.Get(),
											&getAsyncOperation));

		GetTask(getAsyncOperation.Get()).then(
			[this, url, headers, taskCompletionEvent, tryCount]
			(task<ComPtr<IHttpResponseMessage>> task)
			{
				try
				{
					taskCompletionEvent.set(task.get());
				}
				catch (const exception& ex)
				{
					if (tryCount == _maxRetryCount)
					{
						taskCompletionEvent.set_exception(ex);
					}
					else
					{
						GetAsync(url,
								 headers,
								 taskCompletionEvent,
								 tryCount + 1);
					}
				}
			});
	}

	void PostAsync(shared_ptr<wstring> url,
				   shared_ptr<unordered_map<wstring, wstring>> headers,
				   shared_ptr<wstring> postContent,
				   task_completion_event<ComPtr<IHttpResponseMessage>> taskCompletionEvent,
				   uint_fast16_t tryCount) const
	{
		ComPtr<IHttpStringContentFactory> stringContentFactory;
		ComPtr<IHttpContent> postHttpContent;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> postAsyncOperation;

		HString hstringPostContent(*postContent);

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpStringContent).Get(),
								   &stringContentFactory));

		auto contentTypeHeaderIterator = headers->find(HttpRequestHeaders::ContentType);
		if (contentTypeHeaderIterator != headers->end())
		{
			Check(stringContentFactory->CreateFromStringWithEncodingAndMediaType(hstringPostContent.Get(),
																				 UnicodeEncoding_Utf8,
																				 HString(contentTypeHeaderIterator->second).Get(),
																				 &postHttpContent));
			headers->erase(HttpRequestHeaders::ContentType);
		}
		else
		{
			Check(stringContentFactory->CreateFromString(hstringPostContent.Get(),
														 &postHttpContent));
		}

		auto httpRequestMessage = CreateRequest(_httpPostMethod.Get(),
												*url,
												*headers);

		Check(httpRequestMessage->put_Content(postHttpContent.Get()));

		Check(_httpClient->SendRequestAsync(httpRequestMessage.Get(),
											&postAsyncOperation));

		GetTask(postAsyncOperation.Get()).then(
			[this, url, headers, postContent, taskCompletionEvent, tryCount]
			(task<ComPtr<IHttpResponseMessage>> task)
			{
				try
				{
					taskCompletionEvent.set(task.get());
				}
				catch (const exception& ex)
				{
					if (tryCount == _maxRetryCount)
					{
						taskCompletionEvent.set_exception(ex);
					}
					else
					{
						PostAsync(url,
								  headers,
								  postContent,
								  taskCompletionEvent,
								  tryCount + 1);
					}
				}
			});
	}
};

template <>
AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::HttpClient() NOEXCEPT
	: _impl(new HttpClientImpl()) {}

template <>
AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::~HttpClient() NOEXCEPT {}

task<ComPtr<IHttpResponseMessage>> AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::GetAsync(wstring url,
																												unordered_map<wstring, wstring> headers) const
{
	return _impl->GetAsync(url,
						   headers);
}

task<ComPtr<IHttpResponseMessage>> AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>::PostAsync(wstring url,
																												 wstring postContent,
																												 unordered_map<wstring, wstring> headers) const
{
	return _impl->PostAsync(url,
							postContent,
							headers);
}
