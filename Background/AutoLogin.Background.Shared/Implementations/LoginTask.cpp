#include <pch.h>
#include "LoginTask.h"
#include <robuffer.h>
#include <Windows.Data.Xml.Dom.h>
#include <Windows.UI.Notifications.h>
#include <windows.web.http.h>
#include <windows.storage.streams.h>
#include <windows.networking.connectivity.h>
#include <gumbo\gumbo.h>
#include <MTL.h>

using namespace AutoLogin::Background::Implementations;
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

HRESULT LoginTask::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
	*className = HString(RuntimeClass_AutoLogin_Background_LoginTask).Detach();
	return S_OK;
}

HRESULT LoginTask::Run(IBackgroundTaskInstance* taskInstance) NOEXCEPT
{
	try
	{
		ComPtr<IBackgroundTaskDeferral> taskDefferal;
		Check(taskInstance->GetDeferral(&taskDefferal));

		auto connectionProfile = GetNetworkConnectionProfile();

		if (CheckProfile(connectionProfile.Get()))
		{
			ComPtr<IClosable> closableClient;

			auto httpClient = CreateHttpClient();

			Check(httpClient.As(&closableClient));

			GetInitialResponseTask(httpClient.Get()).then(
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
														(IHttpResponseMessage* postResponse)
														{
															PromtSuccessNotification();
														})
													.wait();
		}

		Check(taskDefferal->Complete());

		return S_OK;
	}
	catch (const ComException& comException)
	{
		PromtFailNotification();
		return comException.GetResult();
	}
}

ComPtr<IConnectionProfile> LoginTask::GetNetworkConnectionProfile() NOEXCEPT
{
	ComPtr<INetworkInformationStatics> networkInformationStatics;
	ComPtr<IConnectionProfile> connectionProfile;

	try
	{
		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(),
								   &networkInformationStatics));

		Check(networkInformationStatics->GetInternetConnectionProfile(&connectionProfile));
	}
	catch (const ComException&) { }

	return connectionProfile;
}

bool LoginTask::CheckProfile(IConnectionProfile* connectionProfile) NOEXCEPT
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

ComPtr<IHttpClient> LoginTask::CreateHttpClient()
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

void LoginTask::PromtSuccessNotification()
{
	ComPtr<IToastNotificationManagerStatics> toastNotificationManagerStatics;
	ComPtr<IXmlDocument> xmlDocument;
	ComPtr<IXmlNodeList> xmlNodeList;
	ComPtr<IXmlNode> xmlNode0;
	ComPtr<IXmlNodeSerializer> xmlNodeSerializer0;
	ComPtr<IXmlNode> xmlNode1;
	ComPtr<IXmlNodeSerializer> xmlNodeSerializer1;
	ComPtr<IToastNotificationFactory> toastNotificationFactory;
	ComPtr<IToastNotification> toastNotification;
	ComPtr<IToastNotifier> toastNotifier;

	Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
							   &toastNotificationManagerStatics));

	Check(toastNotificationManagerStatics->GetTemplateContent(ToastTemplateType_ToastText02,
															  &xmlDocument));

	Check(xmlDocument->GetElementsByTagName(HStringReference(L"text").Get(),
											&xmlNodeList));

	Check(xmlNodeList->Item(0, &xmlNode0));

	Check(xmlNode0.As(&xmlNodeSerializer0));

	Check(xmlNodeSerializer0->put_InnerText(HStringReference(L"AutoLogin").Get()));

	Check(xmlNodeList->Item(1, &xmlNode1));

	Check(xmlNode1.As(&xmlNodeSerializer1));

	Check(xmlNodeSerializer1->put_InnerText(HStringReference(L"Соединение установлено").Get()));

	Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
							   &toastNotificationFactory));

	Check(toastNotificationFactory->CreateToastNotification(xmlDocument.Get(),
															&toastNotification));

	Check(toastNotificationManagerStatics->CreateToastNotifier(&toastNotifier));

	Check(toastNotifier->Show(toastNotification.Get()));
}

void LoginTask::PromtFailNotification()
{
	try
	{
		ComPtr<IToastNotificationManagerStatics> toastNotificationManagerStatics;
		ComPtr<IXmlDocument> xmlDocument;
		ComPtr<IXmlNodeList> xmlNodeList;
		ComPtr<IXmlNode> xmlNode0;
		ComPtr<IXmlNodeSerializer> xmlNodeSerializer0;
		ComPtr<IXmlNode> xmlNode1;
		ComPtr<IXmlNodeSerializer> xmlNodeSerializer1;
		ComPtr<IToastNotificationFactory> toastNotificationFactory;
		ComPtr<IToastNotification> toastNotification;
		ComPtr<IToastNotifier> toastNotifier;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
								   &toastNotificationManagerStatics));

		Check(toastNotificationManagerStatics->GetTemplateContent(ToastTemplateType_ToastText02,
																  &xmlDocument));

		Check(xmlDocument->GetElementsByTagName(HStringReference(L"text").Get(),
												&xmlNodeList));

		Check(xmlNodeList->Item(0, &xmlNode0));

		Check(xmlNode0.As(&xmlNodeSerializer0));

		Check(xmlNodeSerializer0->put_InnerText(HStringReference(L"AutoLogin").Get()));

		Check(xmlNodeList->Item(1, &xmlNode1));

		Check(xmlNode1.As(&xmlNodeSerializer1));

		Check(xmlNodeSerializer1->put_InnerText(HStringReference(L"Ошибка соединения").Get()));

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
								   &toastNotificationFactory));

		Check(toastNotificationFactory->CreateToastNotification(xmlDocument.Get(),
																&toastNotification));

		Check(toastNotificationManagerStatics->CreateToastNotifier(&toastNotifier));

		Check(toastNotifier->Show(toastNotification.Get()));
	}
	catch (...) {}
}

task<IHttpResponseMessage*> LoginTask::GetInitialResponseTask(IHttpClient* httpClient)
{
	ComPtr<IUriRuntimeClassFactory> uriFactory;
	ComPtr<IUriRuntimeClass> uri;
	ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;

	Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
							   &uriFactory));

	Check(uriFactory->CreateUri(HStringReference(L"https://www.google.ru").Get(),
								&uri));

	Check(httpClient->GetAsync(uri.Get(),
							   &getAsyncOperation));

	return GetTask(getAsyncOperation.Get());
}

task<IBuffer*> LoginTask::GetResponseContentTask(IHttpResponseMessage* response)
{
	ComPtr<IHttpContent> httpContent;
	ComPtr<IAsyncOperationWithProgress<IBuffer*, ULONGLONG>> readAsBufferOperation;

	Check(response->get_Content(&httpContent));

	Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

	return GetTask(readAsBufferOperation.Get());
}

ComPtr<IUriRuntimeClass> LoginTask::GetResponseLocationHeader(IHttpResponseMessage* response)
{
	ComPtr<IHttpResponseHeaderCollection> responseHeaderCollection;
	ComPtr<IUriRuntimeClass> locationUri;

	Check(response->get_Headers(&responseHeaderCollection));

	Check(responseHeaderCollection->get_Location(&locationUri));

	return locationUri;
}

wstring LoginTask::GetPostContent(IBuffer* responseContent)
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

task<IHttpResponseMessage*> LoginTask::GetAuthResponseTask(IHttpClient* httpClient,
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
