#include <pch.h>
#include <HttpClient.h>
#include <algorithm>
#include <vector>
#include <tuple>
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
using TContentType = THttpClient::TContentType;
using TPostContent = THttpClient::TPostContent;

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
								   &_uriFactory));

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

	task<TResponse> GetAsync(const TUrl &url,
							 const THeaders &headers) const
	{
		task_completion_event<TResponse> taskCompletionEvent;

		GetAsync(CreateUri(HStringReference(url).Get()),
				 CreateHeaders(headers),
				 taskCompletionEvent);

		return task<TResponse>(taskCompletionEvent);
	}

	task<TResponse> PostAsync(const TUrl &url,
							  const THeaders &headers,
							  const TPostContent &postContent) const
	{
		task_completion_event<TResponse> taskCompletionEvent;

		PostAsync(CreateUri(HStringReference(url).Get()),
				  CreateHeaders(headers),
				  CreatePostContent(postContent),
				  taskCompletionEvent);

		return task<TResponse>(taskCompletionEvent);
	}

private:
	uint_fast16_t _maxRetryCount;

	ComPtr<IUriRuntimeClassFactory> _uriFactory;
	ComPtr<IHttpClient> _httpClient;
	ComPtr<IHttpRequestMessageFactory> _httpRequestMessageFactory;
	ComPtr<IHttpMethod> _httpGetMethod;
	ComPtr<IHttpMethod> _httpPostMethod;

	static shared_ptr<vector<tuple<HString, HString>>> CreateHeaders(const THeaders &headers) NOEXCEPT
	{
		auto result = make_shared<vector<tuple<HString, HString>>>();
		for (auto &header : headers)
		{
			result->emplace_back(HString(header.first), HString(header.second));
		}
		return result;
	}

	static shared_ptr<tuple<HString, HString>> CreatePostContent(const TPostContent &postContent) NOEXCEPT
	{
		return make_shared<tuple<HString, HString>>(make_tuple(HString(get<0>(postContent)), HString(get<1>(postContent))));
	}

	ComPtr<IUriRuntimeClass> CreateUri(HSTRING urlString) const
	{
		ComPtr<IUriRuntimeClass> uri;
		Check(_uriFactory->CreateUri(urlString, &uri));
		return uri;
	}

	ComPtr<IHttpRequestMessage> CreateRequest(IHttpMethod *method,
											  IUriRuntimeClass *uri,
											  const vector<tuple<HString, HString>> &headers) const
	{
		ComPtr<IHttpRequestMessage> httpRequestMessage;
		ComPtr<IHttpRequestHeaderCollection> httpRequestHeaderCollection;

		Check(_httpRequestMessageFactory->Create(method, uri, &httpRequestMessage));

		Check(httpRequestMessage->get_Headers(&httpRequestHeaderCollection));

		for (auto &pair : headers)
		{
			boolean isSuccess;
			Check(httpRequestHeaderCollection->TryAppendWithoutValidation(get<0>(pair).Get(),
																		  get<1>(pair).Get(),
																		  &isSuccess));
		}

		return httpRequestMessage;
	}

	void GetAsync(ComPtr<IUriRuntimeClass> uri,
				  shared_ptr<vector<tuple<HString, HString>>> pHeaders,
				  task_completion_event<TResponse> taskCompletionEvent,
				  uint_fast16_t attempt = 1) const
	{
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;

		Check(_httpClient->SendRequestAsync(CreateRequest(_httpGetMethod.Get(), uri.Get(), *pHeaders).Get(),
											&getAsyncOperation));

		GetTask(getAsyncOperation.Get()).then([this, uri, pHeaders, taskCompletionEvent, attempt] (const task<TResponse> &task) mutable
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
						GetAsync(move(uri), move(pHeaders), move(taskCompletionEvent), attempt + 1);
					}
				}
			});
	}

	void PostAsync(ComPtr<IUriRuntimeClass> uri,
				   shared_ptr<vector<tuple<HString, HString>>> pHeaders,
				   shared_ptr<tuple<HString, HString>> postContent,
				   task_completion_event<TResponse> taskCompletionEvent,
				   uint_fast16_t attempt = 1) const
	{
		ComPtr<IHttpStringContentFactory> stringContentFactory;
		ComPtr<IHttpContent> postHttpContent;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> postAsyncOperation;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpStringContent).Get(),
								   &stringContentFactory));

		auto httpRequestMessage = CreateRequest(_httpGetMethod.Get(), uri.Get(), *pHeaders);

		auto& postContentType = get<0>(*postContent);

		if (!postContentType.Empty())
		{
			Check(stringContentFactory->CreateFromStringWithEncodingAndMediaType(get<1>(*postContent).Get(),
																				 UnicodeEncoding_Utf8,
																				 postContentType.Get(),
																				 &postHttpContent));

			Check(httpRequestMessage->put_Content(postHttpContent.Get()));
		}

		Check(_httpClient->SendRequestAsync(httpRequestMessage.Get(), &postAsyncOperation));

		GetTask(postAsyncOperation.Get()).then([this, uri, pHeaders, postContent, taskCompletionEvent, attempt](const task<TResponse> &task) mutable
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
						PostAsync(move(uri), move(pHeaders), move(postContent), move(taskCompletionEvent), attempt + 1);
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
task<TResponse> THttpClient::GetAsync(const TUrl &url,
									  const THeaders &headers) const
{
	return _impl->GetAsync(url, headers);
}

template <>
task<TResponse> THttpClient::PostAsync(const TUrl &url,
									   const THeaders &headers,
									   const TPostContent &postContent) const
{
	return _impl->PostAsync(url, headers, postContent);
}
