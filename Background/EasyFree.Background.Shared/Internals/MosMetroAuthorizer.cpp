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

		string result;

		auto children = &formNode->v.element.children;
		for (unsigned i = 0; i < children->length; ++i)
		{
			auto child = static_cast<GumboNode*>(children->data[i]);
			if (GUMBO_TAG_INPUT == child->v.element.tag)
			{
				string name,
					value;
				auto attributes = &child->v.element.attributes;
				for (unsigned j = 0; j < attributes->length; ++j)
				{
					auto attribute = static_cast<GumboAttribute*>(attributes->data[j]);
					if (strcmp(attribute->name, "name") == 0)
					{
						name.append(attribute->value);
					}
					if (strcmp(attribute->name, "value") == 0)
					{
						value.append(attribute->value);
					}
					//Ключ и значение найдены, можно прерывать цикл
					if (!name.empty() && !value.empty())
					{
						break;
					}
				}

				result.append(name)
					.append("=")
					.append(value)
					.append("&");
			}
		}

		if (!result.empty())
		{
			result = result.substr(0, result.size() - 1);
		}

		return result;
	}
};

class MosMetroAuthorizerImpl final
{
public:
	static task<AuthStatus> Authorize()
	{
		try
		{
			return GetAsync(CreateHttpClient(true).Get())
				.then([](IHttpResponseMessage* response) -> task<AuthStatus>
			{
				ComPtr<IHttpRequestMessage> request;
				Check(response->get_RequestMessage(&request));

				ComPtr<IUriRuntimeClass> locationUri;
				Check(request->get_RequestUri(&locationUri));

				HString absoluteUri;
				Check(locationUri->get_AbsoluteUri(&absoluteUri));

				const wchar_t compareUri[] = L"https://login.wi-fi.ru/am/UI/Login";
				const auto compareUriExtent = extent<decltype(compareUri)>::value - 1;

				if (absoluteUri.Size() < compareUriExtent ||
					wcsncmp(absoluteUri.GetRawBuffer(), compareUri, compareUriExtent) != 0)
				{
					return task_from_result(AuthStatus::None);
				}

				return GetContentAsync(response)
					.then([](IBuffer* responseContent) { return GetPostContent(responseContent); })
					.then([locationUri](wstring postContent) -> task<AuthStatus>
				{
					auto httpClient = CreateHttpClient(false);

					return AuthAsync(httpClient.Get(),
									 locationUri.Get(),
									 move(postContent))
						.then([httpClient](IHttpResponseMessage* authResponse) { return GetAsync(httpClient.Get()); })
						.then([](task<IHttpResponseMessage*> checkResponse) -> AuthStatus
					{
						try
						{
							auto response = checkResponse.get();
							if (nullptr != response)
							{
								boolean isSuccessStatusCode;
								response->get_IsSuccessStatusCode(&isSuccessStatusCode);

								if (isSuccessStatusCode > 0) return AuthStatus::Success;
							}
						}
						catch (...) { }

						return AuthStatus::Fail;
					});
				});
			});
		}
		catch (const ComException&)
		{
			return task_from_result(AuthStatus::None);
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

	static task<IHttpResponseMessage*> GetAsync(IHttpClient* httpClient)
	{
		ComPtr<IUriRuntimeClassFactory> uriFactory;
		ComPtr<IUriRuntimeClass> uri;
		ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
								   &uriFactory));

		Check(uriFactory->CreateUri(HStringReference(L"https://vmet.ro").Get(),
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

task<AuthStatus> MosMetroAuthorizer::Authorize() const NOEXCEPT
{
	return MosMetroAuthorizerImpl::Authorize();
}

bool MosMetroAuthorizer::CanAuth(IConnectionProfile* connectionProfile) const NOEXCEPT
{
	return MosMetroAuthorizerImpl::CanAuth(connectionProfile);
}
