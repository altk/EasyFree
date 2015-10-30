#include "pch.h"
#include "MosMetroAuthorizer.h"
#include <robuffer.h>
#include <Windows.Data.Xml.Dom.h>
#include <Windows.UI.Notifications.h>
#include <windows.web.http.h>
#include <windows.storage.streams.h>
#include <windows.networking.connectivity.h>
#include <gumbo\gumbo.h>
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
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace Windows::Storage::Streams;
using namespace MTL;
using namespace EasyFree::Background::Internals;

class ResponseParser final
{
public:
	static string GetPostString(const char* source) NOEXCEPT
	{
		return ParseHTML(source);
	}

private:
	static string ParseHTML(const char* source) NOEXCEPT
	{
		using namespace std;

		string result;

		if (!source) return result;

		auto output = gumbo_parse(source);

		if (!output) return result;

		auto formNode = FindNodeByTag(output->root,
									  GUMBO_TAG_FORM);

		if (!formNode) return result;

		result = ParseFormElement(formNode);

		gumbo_destroy_output(&kGumboDefaultOptions,
							 output);

		return result;
	}

	static const GumboNode* FindNodeByTag(const GumboNode* node,
										  GumboTag tag) NOEXCEPT
	{
		if (node->v.element.tag == tag)
		{
			return node;
		}

		auto children = &node->v.element.children;
		for (unsigned i = 0; i < children->length; ++i)
		{
			auto child = static_cast<GumboNode*>(children->data[i]);
			if (GUMBO_NODE_ELEMENT == child->type)
			{
				auto result = FindNodeByTag(child, tag);
				if (nullptr != result)
				{
					return result;
				}
			}
		}

		return nullptr;
	}

	static string ParseFormElement(const GumboNode* formNode) NOEXCEPT
	{
		using namespace std;

		vector<tuple<const char*, const char*>> params;

		auto children = &formNode->v.element.children;
		for (unsigned i = 0; i < children->length; ++i)
		{
			auto child = static_cast<GumboNode*>(children->data[i]);
			if (GUMBO_TAG_INPUT == child->v.element.tag)
			{
				tuple<const char*, const char*> idValueTuple;
				auto attributes = &child->v.element.attributes;
				for (unsigned j = 0; j < attributes->length; ++j)
				{
					auto attribute = static_cast<GumboAttribute*>(attributes->data[j]);
					if (strcmp(attribute->name, "name") == 0)
					{
						get<0>(idValueTuple) = attribute->value;
					}
					if (strcmp(attribute->name, "value") == 0)
					{
						get<1>(idValueTuple) = attribute->value;
					}
					//Ключ и значение найдены, можно прерывать цикл
					if (!get<0>(idValueTuple) && !get<1>(idValueTuple))
					{
						break;
					}
				}
				params.emplace_back(move(idValueTuple));
			}
		}

		string result;

		if (!params.empty())
		{
			for (auto& t : params)
			{
				result = result.append(get<0>(t))
							   .append("=")
							   .append(get<1>(t))
							   .append("&");
			}

			result = result.substr(0, result.size() - 1);
		}

		return result;
	}
};

class MosMetroAuthorizerImpl final
{
public:
	static task<bool> Authorize() 
	{
		try
		{
			ComPtr<IClosable> closableClient;

			auto httpClient = CreateHttpClient();

			Check(httpClient.As(&closableClient));

			return GetInitialResponseTask(httpClient.Get()).then(
				[httpClient]
			(IHttpResponseMessage* response) ->
				task<IHttpResponseMessage*>
			{
				auto locationUri = GetResponseLocationHeader(response);
				return GetResponseContentTask(response).then(
					[]
				(IBuffer* responseContent)
				{
					return GetPostContent(responseContent);
				})
					.then(
						[httpClient, locationUri]
				(wstring postContent)
				{
					return GetAuthResponseTask(httpClient.Get(),
						locationUri.Get(),
						move(postContent));
				});
			})
				.then(
					[]
			(task<IHttpResponseMessage*> postResponseTask)->
				bool
			{
				try
				{
					return postResponseTask.get() != nullptr;
				}
				catch (...)
				{
					return false;
				}
			});
		}
		catch (const ComException&)
		{
			return task_from_result(false);
		}
	}

	static bool CanAuth(IConnectionProfile* connectionProfile) NOEXCEPT
	{
		if (!connectionProfile) return false;

		try
		{
			HString profileName;
			Check(connectionProfile->get_ProfileName(&profileName));

			return wcscmp(profileName.GetRawBuffer(), L"MosMetro_Free") == 0;
		}
		catch (const ComException&)
		{
			return false;
		}
	}

private:

	static ComPtr<IHttpClient> CreateHttpClient()
	{
		ComPtr<IHttpClientFactory> httpClientFactory;
		ComPtr<IHttpFilter> httpFilter;
		ComPtr<IHttpBaseProtocolFilter> httpBaseProtocolFilter;
		ComPtr<IHttpClient> httpClient;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpClient).Get(),
								   &httpClientFactory));

		Check(ActivateInstance<IHttpBaseProtocolFilter>(HStringReference(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter).Get(),
														&httpBaseProtocolFilter));

		Check(httpBaseProtocolFilter->put_AllowAutoRedirect(true));

		Check(httpBaseProtocolFilter.As(&httpFilter));

		Check(httpClientFactory->Create(httpFilter.Get(),
										&httpClient));

		return httpClient;
	}

	static task<IHttpResponseMessage*> GetInitialResponseTask(IHttpClient* httpClient)
	{
		ComPtr<IUriRuntimeClassFactory> uriFactory;
		ComPtr<IUriRuntimeClass> uri;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
								   &uriFactory));

		Check(uriFactory->CreateUri(HStringReference(L"https://login.wi-fi.ru/am/UI/Login").Get(),
									&uri));

		Check(httpClient->GetAsync(uri.Get(),
								   &getAsyncOperation));

		return GetTask(getAsyncOperation.Get());
	}

	static task<IBuffer*> GetResponseContentTask(IHttpResponseMessage* response)
	{
		ComPtr<IHttpContent> httpContent;
		ComPtr<IAsyncOperationWithProgress<IBuffer*, ULONGLONG>> readAsBufferOperation;

		Check(response->get_Content(&httpContent));

		Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

		return GetTask(readAsBufferOperation.Get());
	}

	static ComPtr<IUriRuntimeClass> GetResponseLocationHeader(IHttpResponseMessage* response)
	{
		ComPtr<IHttpResponseHeaderCollection> responseHeaderCollection;
		ComPtr<IUriRuntimeClass> locationUri;

		Check(response->get_Headers(&responseHeaderCollection));

		Check(responseHeaderCollection->get_Location(&locationUri));

		return locationUri;
	}

	static wstring GetPostContent(IBuffer* responseContent)
	{
		auto buffer = CreateComPtr(responseContent);

		ComPtr<IBufferByteAccess> bufferByteAccess;
		Check(buffer.As(&bufferByteAccess));

		byte* content;
		Check(bufferByteAccess->Buffer(&content));

		UINT32 lenght;
		Check(buffer->get_Length(&lenght));

		auto postContent = ResponseParser::GetPostString(reinterpret_cast<const char*>(content));
		return wstring(begin(postContent), end(postContent));
	}

	static task<IHttpResponseMessage*> GetAuthResponseTask(IHttpClient* httpClient,
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

task<bool> MosMetroAuthorizer::Authorize() const NOEXCEPT
{
	return MosMetroAuthorizerImpl::Authorize();
}

bool MosMetroAuthorizer::CanAuth(IConnectionProfile* connectionProfile) const NOEXCEPT 
{
	return MosMetroAuthorizerImpl::CanAuth(connectionProfile);
}