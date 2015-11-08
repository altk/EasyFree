#include <pch.h>
#include "LoginTask.h"
#include <Windows.Data.Xml.Dom.h>
#include <Windows.UI.Notifications.h>
#include <MTL.h>
#include <Internals/MosMetroAuthorizer.h>
#include <Internals/NetworkInfoProvider.h>

using namespace EasyFree::Background::Implementations;
using namespace std;
using namespace Concurrency;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace MTL;

const wchar_t launchAttributeName[] = L"launch";
const wchar_t textTagName[] = L"text";
const wchar_t launchAttributeSuccess[] = L"success";
const wchar_t launchAttributeFail[] = L"fail";
const wchar_t launchAttributeUnauthorized[] = L"unauthorized";
const wchar_t titleText[] = L"Easy Free";
const wchar_t successText[] = L"Соединение устрановлено";
const wchar_t failText[] = L"Ошибка соедиения";
const wchar_t unauthorizedText[] = L"Необходима авторизация";

class NotificationHelper final
{
public:
	static void PromtSuccessNotification()
	{
		ComPtr<IToastNotificationManagerStatics> toastNotificationManagerStatics;
		ComPtr<IXmlAttribute> launchAttribute;
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

		Check(xmlDocument->CreateAttribute(HStringReference(launchAttributeName).Get(),
										   &launchAttribute));

		Check(launchAttribute->put_Value(HStringReference(launchAttributeSuccess).Get()));

		Check(xmlDocument->GetElementsByTagName(HStringReference(textTagName).Get(),
												&xmlNodeList));

		Check(xmlNodeList->Item(0, &xmlNode0));

		Check(xmlNode0.As(&xmlNodeSerializer0));

		Check(xmlNodeSerializer0->put_InnerText(HStringReference(titleText).Get()));

		Check(xmlNodeList->Item(1, &xmlNode1));

		Check(xmlNode1.As(&xmlNodeSerializer1));

		Check(xmlNodeSerializer1->put_InnerText(HStringReference(successText).Get()));

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
								   &toastNotificationFactory));

		Check(toastNotificationFactory->CreateToastNotification(xmlDocument.Get(),
																&toastNotification));

		Check(toastNotificationManagerStatics->CreateToastNotifier(&toastNotifier));

		Check(toastNotifier->Show(toastNotification.Get()));
	}

	static void PromtFailNotification()
	{
		ComPtr<IToastNotificationManagerStatics> toastNotificationManagerStatics;
		ComPtr<IXmlAttribute> launchAttribute;
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

		Check(xmlDocument->CreateAttribute(HStringReference(launchAttributeName).Get(),
										   &launchAttribute));

		Check(launchAttribute->put_Value(HStringReference(launchAttributeFail).Get()));

		Check(xmlDocument->GetElementsByTagName(HStringReference(textTagName).Get(),
												&xmlNodeList));

		Check(xmlNodeList->Item(0, &xmlNode0));

		Check(xmlNode0.As(&xmlNodeSerializer0));

		Check(xmlNodeSerializer0->put_InnerText(HStringReference(titleText).Get()));

		Check(xmlNodeList->Item(1, &xmlNode1));

		Check(xmlNode1.As(&xmlNodeSerializer1));

		Check(xmlNodeSerializer1->put_InnerText(HStringReference(failText).Get()));

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
								   &toastNotificationFactory));

		Check(toastNotificationFactory->CreateToastNotification(xmlDocument.Get(),
																&toastNotification));

		Check(toastNotificationManagerStatics->CreateToastNotifier(&toastNotifier));

		Check(toastNotifier->Show(toastNotification.Get()));
	}

	static void PromtUnauthorizedNotification()
	{
		ComPtr<IToastNotificationManagerStatics> toastNotificationManagerStatics;
		ComPtr<IXmlAttribute> launchAttribute;
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

		Check(xmlDocument->CreateAttribute(HStringReference(launchAttributeName).Get(),
										   &launchAttribute));

		Check(launchAttribute->put_Value(HStringReference(launchAttributeUnauthorized).Get()));

		Check(xmlDocument->GetElementsByTagName(HStringReference(textTagName).Get(),
												&xmlNodeList));

		Check(xmlNodeList->Item(0, &xmlNode0));

		Check(xmlNode0.As(&xmlNodeSerializer0));

		Check(xmlNodeSerializer0->put_InnerText(HStringReference(titleText).Get()));

		Check(xmlNodeList->Item(1, &xmlNode1));

		Check(xmlNode1.As(&xmlNodeSerializer1));

		Check(xmlNodeSerializer1->put_InnerText(HStringReference(unauthorizedText).Get()));

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
								   &toastNotificationFactory));

		Check(toastNotificationFactory->CreateToastNotification(xmlDocument.Get(),
																&toastNotification));

		Check(toastNotificationManagerStatics->CreateToastNotifier(&toastNotifier));

		Check(toastNotifier->Show(toastNotification.Get()));
	}
};

HRESULT LoginTask::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
	*className = HString(RuntimeClass_EasyFree_Background_LoginTask).Detach();
	return S_OK;
}

HRESULT LoginTask::Run(IBackgroundTaskInstance* taskInstance) NOEXCEPT
{
	using namespace Internals;

	MosMetroAuthorizer authorizer;
	if (authorizer.CanAuth(NetworkInfoProvider::GetNetworkConnectionProfile().Get()))
	{
		ComPtr<IBackgroundTaskDeferral> taskDefferal;
		Check(taskInstance->GetDeferral(&taskDefferal));

		authorizer.Authorize()
			.then([taskDefferal](AuthStatus authResult) NOEXCEPT->void
		{
			try
			{
				switch (authResult)
				{
					case AuthStatus::Success:
						NotificationHelper::PromtSuccessNotification();
						break;
					case AuthStatus::Fail:
						NotificationHelper::PromtFailNotification();
						break;
				}

				Check(taskDefferal->Complete());
			}
			catch (...) { }
		});
	}

	return S_OK;
}