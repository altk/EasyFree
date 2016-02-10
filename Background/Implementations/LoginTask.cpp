#include <pch.h>
#include "LoginTask.h"
#include <Windows.Data.Xml.Dom.h>
#include <Windows.UI.Notifications.h>
#include <windows.applicationmodel.background.h>
#include <windows.web.http.h>
#include <MTL.h>
#include <MosMetroAuthorizer.h>
#include <NetworkInfoProvider.h>
#include <Labels.h>
#include <AuthStatus.h>
#include <UriUtilities.h>

using namespace std;
using namespace Concurrency;
using namespace ABI::Windows::Web::Http;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace MTL::Internals;
using namespace MTL;
using namespace AutoLogin::Background::Implementations;
using namespace AutoLogin::CrossPlatform;
using namespace AutoLogin::Windows;
using namespace AutoLogin::Resources;

struct NotificationHelper final
{
	static void PromtNotification(const wstring &title,
								  const wstring &launchAttribute,
								  const wstring &description)
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

			auto toastXml = wstring(L"<toast launch=\"").append(launchAttribute)
														.append(L"\"><visual><binding template=\"ToastText02\"><text id=\"1\">")
														.append(title)
														.append(L"</text><text id=\"2\">")
														.append(description)
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

HRESULT LoginTask::GetRuntimeClassName(HSTRING *className) NOEXCEPT
{
	*className = HString(RuntimeClass_AutoLogin_Background_LoginTask).Detach();
	return S_OK;
}

HRESULT LoginTask::Run(IBackgroundTaskInstance *taskInstance) NOEXCEPT
{
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
			auto connectionNameStr = wstring(connectionName.GetRawBuffer());

			auto findIterator = find_if(begin(authorizers),
										end(authorizers),
										[&connectionNameStr]
										(const shared_ptr<IAuthorizer> &authorizer) ->
										bool
										{
											return authorizer->CanAuth(connectionNameStr);
										});

			if (findIterator != end(authorizers))
			{
				auto authorizer = *findIterator;

				ComPtr<IBackgroundTaskDeferral> taskDefferal;
				Check(taskInstance->GetDeferral(&taskDefferal));

				authorizer->AuthAsync().then(
					[connectionNameStr, taskDefferal, authorizer]
					(task<AuthResult> authResultTask) NOEXCEPT ->
					void
					{
						try
						{
							const wstring *launchArgument,
										  *description,
										  *title;
							wstring temp;

							switch (authResultTask.get())
							{
								case AuthResult::Success:
									title = &connectionNameStr;
									launchArgument = &AuthStatus::launchAttributeSuccess;
									description = &Labels::AuthSuccess;
									break;
								case AuthResult::Fail:
									title = &connectionNameStr;
									launchArgument = &AuthStatus::launchAttributeFail;
									description = &Labels::AuthFail;
									break;
								case AuthResult::Unregistered:
									title = &connectionNameStr;
									temp = authorizer->GetRegistrationUrl();
									launchArgument = &temp;
									description = &Labels::RegistrationNeed;
									break;
								case AuthResult::Unlicensed:
									title = &Labels::Title;
									launchArgument = &AuthStatus::launchAttributeUnlicensed;
									description = &Labels::Unlicensed;
									break;
								default:
									return;
							}

							NotificationHelper::PromtNotification(*title,
																  UriUtilities().Escape(*launchArgument)
																				.GetRawBuffer(),
																  *description);
						}
						catch (...)
						{
							NotificationHelper::PromtNotification(Labels::Title,
																  UriUtilities().Escape(AuthStatus::launchAttributeFail)
																				.GetRawBuffer(),
																  Labels::AuthFail);
						}

						Check(taskDefferal->Complete());
					});
			}
		}
	}
	catch (...)
	{
		NotificationHelper::PromtNotification(Labels::Title,
											  UriUtilities().Escape(AuthStatus::launchAttributeFail).GetRawBuffer(),
											  Labels::AuthFail);
	}

	return S_OK;
}
