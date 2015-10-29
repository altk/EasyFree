#include <pch.h>
#include "LoginTask.h"
#include <Windows.Data.Xml.Dom.h>
#include <Windows.UI.Notifications.h>
#include <MTL.h>
#include <Internals/MosMetroAuthorizer.h>
#include <Internals/NetworkInfoProvider.h>
#include <Internals/PackageCkecker.h>

using namespace EasyFree::Background::Implementations;
using namespace std;
using namespace Concurrency;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace MTL;

HRESULT LoginTask::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
	*className = HString(RuntimeClass_EasyFree_Background_LoginTask).Detach();
	return S_OK;
}

HRESULT LoginTask::Run(IBackgroundTaskInstance* taskInstance) NOEXCEPT
{
	using namespace Internals;

	ComPtr<IBackgroundTaskDeferral> taskDefferal;
	Check(taskInstance->GetDeferral(&taskDefferal));

	if (MosMetroAuthorizer().Authorize(NetworkInfoProvider::GetNetworkConnectionProfile().Get()).get())
	{
		PromtSuccessNotification();
	}
	else
	{
		PromtFailNotification();
	}

	Check(taskDefferal->Complete());

	return S_OK;
}

void LoginTask::PromtSuccessNotification() NOEXCEPT
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

		Check(xmlNodeSerializer0->put_InnerText(HStringReference(L"EasyFree").Get()));

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
	catch (...) {}
}

void LoginTask::PromtFailNotification() NOEXCEPT
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

		Check(xmlNodeSerializer0->put_InnerText(HStringReference(L"EasyFree").Get()));

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
