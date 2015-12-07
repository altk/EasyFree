#include <pch.h>
#include "LoginTask.h"
#include <Windows.Data.Xml.Dom.h>
#include <Windows.UI.Notifications.h>
#include <windows.applicationmodel.background.h>
#include <windows.web.http.h>
#include <MTL.h>
#include <MosMetroAuthorizer.h>
#include <NetworkInfoProvider.h>
#include <LicenseChecker.h>
#include <Labels.h>
#include <AuthStatus.h>
#include <UriUtilities.h>
#include <chrono>

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
	static void PromtNotification(const wstring& launchAttribute,
								  const wstring& description)
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
														.append(Labels::Title)
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
#ifdef TEST

	static void PromtStart()
	{
		using namespace chrono;

		auto now = system_clock::to_time_t(system_clock::now());
		tm nowLocal;
		localtime_s(&nowLocal, &now);

		PromtNotification(wstring(),
						  wstring(L"Start ").append(to_wstring(nowLocal.tm_hour))
											.append(L":")
											.append(to_wstring(nowLocal.tm_min))
											.append(L":")
											.append(to_wstring(nowLocal.tm_sec)));
	}

	static void PromtEnd()
	{
		using namespace chrono;

		auto now = system_clock::to_time_t(system_clock::now());
		tm nowLocal;
		localtime_s(&nowLocal, &now);

		PromtNotification(wstring(),
						  wstring(L"End ").append(to_wstring(nowLocal.tm_hour))
										  .append(L":")
										  .append(to_wstring(nowLocal.tm_min))
										  .append(L":")
										  .append(to_wstring(nowLocal.tm_sec)));
	}

	static void PromtException(const exception& ex)
	{
		string message(ex.what());

		PromtNotification(wstring(),
						  wstring(begin(message), end(message)));
	}

#else
	static void PromtStart() { }

	static void PromtEnd() { }

	static void PromtException(exception) { }
#endif 
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
		auto currentNetwork = NetworkInfoProvider::GetNetworkConnectionProfile();
		if (currentNetwork)
		{
			vector<shared_ptr<IAuthorizer>> authorizers
					{
						make_shared<MosMetroAuthorizer<ComPtr<IHttpResponseMessage>>>()
					};

			HString connectionName;
			Check(currentNetwork->get_ProfileName(&connectionName));
			auto connectionNameRaw = connectionName.GetRawBuffer();

			auto findIterator = find_if(begin(authorizers),
										end(authorizers),
										[connectionNameRaw]
										(const shared_ptr<IAuthorizer>& authorizer) ->
										bool
										{
											return authorizer->CanAuth(connectionNameRaw);
										});

			if (findIterator != end(authorizers))
			{
				NotificationHelper::PromtStart();

				if (LicenseChecker::Check())
				{
					auto authorizer = *findIterator;

					ComPtr<IBackgroundTaskDeferral> taskDefferal;
					Check(taskInstance->GetDeferral(&taskDefferal));

					authorizer->AuthAsync()
							.then(
								[taskDefferal, authorizer]
								(task<wstring> authResultTask) NOEXCEPT ->
								void
								{
									try
									{
										auto authResult = authResultTask.get();
										if (!authResult.empty())
										{
											wstring launchArgument,
													description;

											if (authResult == AuthStatus::launchAttributeSuccess)
											{
												launchArgument = AuthStatus::launchAttributeSuccess;
												description = Labels::AuthSuccess;
											}
											else if (authResult == authorizer->GetRegistrationUrl())
											{
												launchArgument = authorizer->GetRegistrationUrl();
												description = Labels::RegistrationNeed;
											}
											else
											{
												launchArgument = authResult;
												description = Labels::AuthFail;
											}

											NotificationHelper::PromtNotification(UriUtilities().Escape(HStringReference(launchArgument.data(), launchArgument.size()).Get()).GetRawBuffer(),
																				  description);

											NotificationHelper::PromtEnd();
										}
									}
									catch (const exception& ex)
									{
										NotificationHelper::PromtException(ex);
									}
									catch (...) {}

									Check(taskDefferal->Complete());
								});
				}
				else
				{
					NotificationHelper::PromtNotification(AuthStatus::launchAttributeUnlicensed,
														  Labels::Unlicensed);
				}
			}
		}
	}
	catch (const exception& ex)
	{
		NotificationHelper::PromtException(ex);
	}
	catch (...) { }

	return S_OK;
}
