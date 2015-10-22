#include <pch.h>
#include <chrono>
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

const GumboNode* findNodeByTag(const GumboNode* node, GumboTag tag) NOEXCEPT
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
			auto result = findNodeByTag(child, tag);
			if (nullptr != result)
			{
				return result;
			}
		}
	}

	return nullptr;
}

std::string getPostContent(const char* source)
{
	using namespace std;

	auto output = gumbo_parse(source);

	auto formNode = findNodeByTag(output->root, GUMBO_TAG_FORM);

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
				if (nullptr != get<0>(idValueTuple) && nullptr != get<1>(idValueTuple))
				{
					break;
				}
			}
			params.push_back(idValueTuple);
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

	gumbo_destroy_output(&kGumboDefaultOptions, output);

	return result;
}

HRESULT LoginTask::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
	using namespace MTL;

	*className = HString(RuntimeClass_AutoLogin_Background_LoginTask).Detach();

	return S_OK;
}

HRESULT LoginTask::Run(ABI::Windows::ApplicationModel::Background::IBackgroundTaskInstance* taskInstance) NOEXCEPT
{
	using namespace ABI::Windows::Data::Xml::Dom;
	using namespace ABI::Windows::UI::Notifications;
	using namespace ABI::Windows::Networking::Connectivity;
	using namespace ABI::Windows::ApplicationModel::Background;
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::Web::Http::Filters;
	using namespace ABI::Windows::Web::Http;
	using namespace ABI::Windows::Storage::Streams;
	using namespace Windows::Storage::Streams;
	using namespace MTL;
	using namespace std::chrono;

	ComPtr<IBackgroundTaskDeferral> taskDefferal;
	taskInstance->GetDeferral(&taskDefferal);

	ComPtr<INetworkInformationStatics> networkInformationStatics;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(),
						 &networkInformationStatics);

	ComPtr<IConnectionProfile> connectionProfile;
	networkInformationStatics->GetInternetConnectionProfile(&connectionProfile);

	if (connectionProfile)
	{
		HString profileName;
		connectionProfile->get_ProfileName(&profileName);

		if (wcscmp(profileName.GetRawBuffer(), L"MosMetro_Free") == 0)
		{
			ComPtr<IHttpClientFactory> httpClientFactory;
			GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpClient).Get(),
								 &httpClientFactory);

			ComPtr<IHttpFilter> httpFilter;
			ActivateInstance<IHttpFilter>(HStringReference(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter).Get(),
										  &httpFilter);

			ComPtr<IHttpClient> httpClient;
			httpClientFactory->Create(httpFilter.Get(),
									  &httpClient);

			ComPtr<IUriRuntimeClassFactory> uriFactory;
			GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
								 &uriFactory);

			ComPtr<IUriRuntimeClass> uri;
			uriFactory->CreateUri(HStringReference(L"https://login.wi-fi.ru/am/UI/Login?org=mac&service=coa&client_mac=c8-d1-0b-01-24-e1&ForceAuth=true").Get(),
								  &uri);

			ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;
			httpClient->GetAsync(uri.Get(),
								 &getAsyncOperation);

			ComPtr<IHttpResponseMessage> response(GetTask(getAsyncOperation.Get()).get());

			ComPtr<IHttpContent> httpContent;
			response->get_Content(&httpContent);

			ComPtr<IAsyncOperationWithProgress<IBuffer*, ULONGLONG>> readAsBufferOperation;
			httpContent->ReadAsBufferAsync(&readAsBufferOperation);

			ComPtr<IBuffer> buffer(GetTask(readAsBufferOperation.Get()).get());

			ComPtr<IBufferByteAccess> bufferByteAccess;
			buffer.As(&bufferByteAccess);

			byte* content;
			bufferByteAccess->Buffer(&content);

			UINT32 lenght;
			buffer->get_Length(&lenght);

			auto postContent = getPostContent(reinterpret_cast<const char*>(content));
			auto wPostContent = std::wstring(postContent.begin(), postContent.end());

			ComPtr<IHttpStringContentFactory> stringContentFactory;
			GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpStringContent).Get(),
								 &stringContentFactory);

			ComPtr<IHttpContent> postHttpContent;
			stringContentFactory->CreateFromString(HString(wPostContent.data(), wPostContent.size()).Get(),
												   &postHttpContent);

			ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> postAsyncOperation;
			httpClient->PostAsync(uri.Get(),
								  postHttpContent.Get(),
								  &postAsyncOperation);

			GetTask(postAsyncOperation.Get()).wait();

#pragma region notification

			ComPtr<IToastNotificationManagerStatics> toastNotificationManagerStatics;
			GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
								 &toastNotificationManagerStatics);

			ComPtr<IXmlDocument> xmlDocument;
			toastNotificationManagerStatics->GetTemplateContent(ToastTemplateType_ToastText02,
																&xmlDocument);

			ComPtr<IXmlNodeList> xmlNodeList;
			xmlDocument->GetElementsByTagName(HStringReference(L"text").Get(),
											  &xmlNodeList);

			ComPtr<IXmlNode> xmlNode0;
			xmlNodeList->Item(0, &xmlNode0);

			ComPtr<IXmlNodeSerializer> xmlNodeSerializer0;
			xmlNode0.As(&xmlNodeSerializer0);

			xmlNodeSerializer0->put_InnerText(HStringReference(L"AutoLogin").Get());

			ComPtr<IXmlNode> xmlNode1;
			xmlNodeList->Item(1, &xmlNode1);

			ComPtr<IXmlNodeSerializer> xmlNodeSerializer1;
			xmlNode1.As(&xmlNodeSerializer1);

			xmlNodeSerializer1->put_InnerText(HStringReference(L"Connection completed").Get());

			ComPtr<IToastNotificationFactory> toastNotificationFactory;
			GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
								 &toastNotificationFactory);

			ComPtr<IToastNotification> toastNotification;
			toastNotificationFactory->CreateToastNotification(xmlDocument.Get(),
															  &toastNotification);

			ComPtr<IToastNotifier> toastNotifier;
			toastNotificationManagerStatics->CreateToastNotifier(&toastNotifier);

			toastNotifier->Show(toastNotification.Get());

#pragma endregion
		}
	}

	taskDefferal->Complete();

	return S_OK;
}
