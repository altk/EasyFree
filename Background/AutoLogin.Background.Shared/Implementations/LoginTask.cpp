#include <pch.h>
#include "LoginTask.h"
#include <Windows.Data.Xml.Dom.h>
#include <Windows.UI.Notifications.h>
#include <MTL.h>
#include <MosMetroAuthorizer.h>
#include <NetworkInfoProvider.h>
#include <LicenseChecker.h>
#include <Resources.h>

using namespace std;
using namespace Concurrency;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace MTL;
using namespace AutoLogin::Background::Implementations;
using namespace AutoLogin::CrossPlatform;
using namespace AutoLogin::Windows;

class NotificationHelper final
{
public:
	static void PromtSuccessNotification()
	{
		PromtNotification(AuthStatus::launchAttributeSuccess,
						  Resources::Title,
						  Resources::AuthSuccess);
	}

	static void PromtFailNotification()
	{
		PromtNotification(AuthStatus::launchAttributeFail,
						  Resources::Title,
						  Resources::AuthFail);
	}

	static void PromtUnauthorizedNotification(string launchAttribute)
	{
		PromtNotification(wstring(begin(launchAttribute), end(launchAttribute)),
						  Resources::Title,
						  Resources::RegistrationNeed);
	}

	static void PromtUnlicensedNotification()
	{
		PromtNotification(AuthStatus::launchAttributeUnlicensed,
						  Resources::Title,
						  Resources::Unlicensed);
	}

private:
	static void PromtNotification(wstring launchAttribute,
								  wstring title,
								  wstring description)
	{
		try
		{
			ComPtr<IToastNotificationManagerStatics> toastNotificationManagerStatics;
			ComPtr<IXmlDocument> document;
			ComPtr<IXmlDocumentIO> documentIO;
			ComPtr<IToastNotificationFactory> toastNotificationFactory;
			ComPtr<IToastNotification> toastNotification;
			ComPtr<IToastNotifier> toastNotifier;

			Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
									   &toastNotificationManagerStatics));

			Check(ActivateInstance<IXmlDocument>(HStringReference(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument).Get(),
												 &document));

			Check(document.As(&documentIO));

			auto toastXml = wstring(L"<toast launch=\"").append(move(launchAttribute))
														.append(L"\"><visual><binding template=\"ToastText02\"><text id=\"1\">")
														.append(move(title))
														.append(L"</text><text id=\"2\">")
														.append(move(description))
														.append(L"</text></binding></visual></toast>");

			Check(documentIO->LoadXml(HStringReference(toastXml.data(), toastXml.size()).Get()));

			Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
									   &toastNotificationFactory));

			Check(toastNotificationFactory->CreateToastNotification(document.Get(),
																	&toastNotification));

			Check(toastNotificationManagerStatics->CreateToastNotifier(&toastNotifier));

			Check(toastNotifier->Show(toastNotification.Get()));
		}
		catch (...) {}
	}
};

HRESULT LoginTask::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
	*className = HString(RuntimeClass_AutoLogin_Background_LoginTask).Detach();
	return S_OK;
}

HRESULT LoginTask::Run(IBackgroundTaskInstance* taskInstance) NOEXCEPT
{
	using namespace std;
	using namespace Internals;

	try
	{
		auto currentNetwork = NetworkInfoProvider::GetNetworkConnectionProfile();
		if (currentNetwork)
		{
			vector<shared_ptr<IAuthorizer>> authorizers
					{
						make_shared<MosMetroAuthorizer>()
					};

			HString connectionName;
			Check(currentNetwork->get_ProfileName(&connectionName));
			auto connectionNameRaw = connectionName.GetRawBuffer();

			auto findIterator = find_if(begin(authorizers),
										end(authorizers),
										[connectionNameRaw](const shared_ptr<IAuthorizer>& authorizer)-> bool
										{
											return authorizer->CanAuth(connectionNameRaw);
										});

			if (findIterator != end(authorizers))
			{
				if (LicenseChecker::Check())
				{
					auto authorizer = *findIterator;

					ComPtr<IBackgroundTaskDeferral> taskDefferal;
					Check(taskInstance->GetDeferral(&taskDefferal));

					authorizer->AuthAsync()
							.then([taskDefferal, authorizer](AuthStatus::Enum authResult) NOEXCEPT-> void
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
											case AuthStatus::Unauthorized:
												NotificationHelper::PromtUnauthorizedNotification(authorizer->GetAuthUrl());
												break;
											default:
												break;
										}

										Check(taskDefferal->Complete());
									}
									catch (...) {}
								});
				}
				else
				{
					NotificationHelper::PromtUnlicensedNotification();
				}
			}
		}
	}
	catch (...) {}

	return S_OK;
}
