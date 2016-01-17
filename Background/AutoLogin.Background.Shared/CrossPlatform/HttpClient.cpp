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

using TResponse = ComPtr<IHttpResponseMessage>;
using THttpClient = AutoLogin::CrossPlatform::HttpClient<TResponse>;
using TUrl = THttpClient::TUrl;
using THeaders = THttpClient::THeaders;
using TContent = THttpClient::TContent;

template <>
class THttpClient::HttpClientImpl final
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

	task<TResponse> GetAsync(TUrl url,
							 THeaders headers) const
	{
		task_completion_event<TResponse> taskCompletionEvent;

		GetAsync(make_shared<TUrl>(move(url)),
				 make_shared<THeaders>(move(headers)),
				 taskCompletionEvent);

		return task<TResponse>(taskCompletionEvent);
	}

	task<TResponse> PostAsync(TUrl url,
							  TContent postContent,
							  THeaders headers) const
	{
		task_completion_event<TResponse> taskCompletionEvent;

		PostAsync(make_shared<TUrl>(move(url)),
				  make_shared<THeaders>(move(headers)),
				  make_shared<TContent>(move(postContent)),
				  taskCompletionEvent);

		return task<TResponse>(taskCompletionEvent);
	}

private:
	uint_fast16_t _maxRetryCount;

	ComPtr<IUriRuntimeClassFactory> uriFactory;
	ComPtr<IHttpClient> _httpClient;
	ComPtr<IHttpRequestMessageFactory> _httpRequestMessageFactory;
	ComPtr<IHttpMethod> _httpGetMethod;
	ComPtr<IHttpMethod> _httpPostMethod;

	ComPtr<IUriRuntimeClass> CreateUri(const TUrl &url) const
	{
		ComPtr<IUriRuntimeClass> uri;
		Check(uriFactory->CreateUri(HStringReference(url).Get(),
									&uri));

		return uri;
	}

	ComPtr<IHttpRequestMessage> CreateRequest(IHttpMethod *method,
											  const TUrl &url,
											  const THeaders &headers) const
	{
		ComPtr<IHttpRequestMessage> httpRequestMessage;
		ComPtr<IHttpRequestHeaderCollection> httpRequestHeaderCollection;

		Check(_httpRequestMessageFactory->Create(method, CreateUri(url).Get(), &httpRequestMessage));

		Check(httpRequestMessage->get_Headers(&httpRequestHeaderCollection));

		for (auto &header : headers)
		{
			boolean isSuccess;
			Check(httpRequestHeaderCollection->TryAppendWithoutValidation(HStringReference(header.first).Get(),
																		  HStringReference(header.second).Get(),
																		  &isSuccess));
		}

		return httpRequestMessage;
	}

	void GetAsync(shared_ptr<TUrl> pUrl,
				  shared_ptr<THeaders> pHeaders,
				  task_completion_event<TResponse> taskCompletionEvent,
				  uint_fast16_t attempt = 1) const
	{
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;

		auto httpRequestMessage = CreateRequest(_httpGetMethod.Get(), *pUrl, *pHeaders);

		Check(_httpClient->SendRequestAsync(httpRequestMessage.Get(), &getAsyncOperation));

		GetTask(getAsyncOperation.Get()).then([this, pUrl, pHeaders, taskCompletionEvent, attempt] (const task<TResponse> &task) mutable
			{
				try
				{
					taskCompletionEvent.set(task.get());
				}
				catch (const exception &ex)
				{
					if (attempt == _maxRetryCount)
					{
						taskCompletionEvent.set_exception(ex);
					}
					else
					{
						GetAsync(move(pUrl), move(pHeaders), move(taskCompletionEvent), attempt + 1);
					}
				}
			});
	}

	void PostAsync(shared_ptr<TUrl> pUrl,
				   shared_ptr<THeaders> pHeaders,
				   shared_ptr<TContent> pPostContent,
				   task_completion_event<TResponse> taskCompletionEvent,
				   uint_fast16_t attempt = 1) const
	{
		ComPtr<IHttpStringContentFactory> stringContentFactory;
		ComPtr<IHttpContent> postHttpContent;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> postAsyncOperation;

		HStringReference hstringPostContent(*pPostContent);

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpStringContent).Get(),
								   &stringContentFactory));

		auto contentTypeHeaderIterator = pHeaders->find(HttpRequestHeaders::ContentType);
		if (contentTypeHeaderIterator != pHeaders->end())
		{
			Check(stringContentFactory->CreateFromStringWithEncodingAndMediaType(hstringPostContent.Get(),
																				 UnicodeEncoding_Utf8,
																				 HStringReference(contentTypeHeaderIterator->second).Get(),
																				 &postHttpContent));
			pHeaders->erase(HttpRequestHeaders::ContentType);
		}
		else
		{
			Check(stringContentFactory->CreateFromString(hstringPostContent.Get(), &postHttpContent));
		}

		auto httpRequestMessage = CreateRequest(_httpPostMethod.Get(), *pUrl, *pHeaders);

		Check(httpRequestMessage->put_Content(postHttpContent.Get()));

		Check(_httpClient->SendRequestAsync(httpRequestMessage.Get(), &postAsyncOperation));

		GetTask(postAsyncOperation.Get()).then([this, pUrl, pHeaders, pPostContent, taskCompletionEvent, attempt](const task<TResponse> &task) mutable
			{
				try
				{
					taskCompletionEvent.set(task.get());
				}
				catch (const exception &ex)
				{
					if (attempt == _maxRetryCount)
					{
						taskCompletionEvent.set_exception(ex);
					}
					else
					{
						PostAsync(move(pUrl), move(pHeaders), move(pPostContent), move(taskCompletionEvent), attempt + 1);
					}
				}
			});
	}
};

template <>
THttpClient::HttpClient() NOEXCEPT
	: _impl(new HttpClientImpl()) {}

template <>
THttpClient::~HttpClient() NOEXCEPT {}

template <>
task<TResponse> THttpClient::GetAsync(TUrl url,
									  THeaders headers) const
{
	return _impl->GetAsync(move(url), move(headers));
}

template <>
task<TResponse> THttpClient::PostAsync(TUrl url,
									   TContent postContent,
									   THeaders headers) const
{
	return _impl->PostAsync(move(url), move(postContent), move(headers));
}
