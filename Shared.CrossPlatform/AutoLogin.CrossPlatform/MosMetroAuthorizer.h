#pragma once
#include <IAuthorizer.h>
#include <AuthStatus.h>
#include <HttpClient.h>
#include <HttpRequestHeaders.h>
#include <SettingsProvider.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		template <typename TResponse>
		class MosMetroAuthorizer final : public IAuthorizer
		{
		public:
			virtual Concurrency::task<AuthResult> AuthAsync() NOEXCEPT override
			{
				using namespace std;
				using namespace Concurrency;
				using namespace Resources;

				try
				{
					const HttpClient<TResponse> httpClient;

					auto checkInternetAvailabilityTask = httpClient.GetAsync(L"http://wi-fi.ru")
																   .then([](TResponse &response)
																	   {
																		   return GetAuthUrlAsync(move(response));
																	   });

					SettingsProvider settingsProvider;
					auto pAuthUrlKey = make_shared<wstring>(L"AuthUrl");
					auto savedAuthUrl = settingsProvider.Get(*pAuthUrlKey);

					task<wstring> getAuthUrlTask;
					if (savedAuthUrl.empty())
					{
						getAuthUrlTask = checkInternetAvailabilityTask.then([settingsProvider, pAuthUrlKey](wstring &authUrl)
							{
								if (!authUrl.empty())
								{
									settingsProvider.Set(*pAuthUrlKey, authUrl);
								}
								return authUrl;
							});
					}
					else
					{
						getAuthUrlTask = task_from_result(savedAuthUrl);
					}

					auto authTask = getAuthUrlTask.then([httpClient](wstring &authUrl) mutable -> task<AuthResult>
						{
							if (authUrl.empty())
							{
								return task_from_result(AuthResult::None);
							}

							if (authUrl == GetRegistrationUrlImpl())
							{
								return task_from_result(AuthResult::Unregistered);
							}

							unordered_map<wstring, wstring> headers
									{
										{
											HttpRequestHeaders::Accept,
											L"text/html"
										},
										{
											HttpRequestHeaders::UserAgent,
											L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36"
										},
										{
											HttpRequestHeaders::Connection,
											L"close"
										}
									};

							auto authUrlPtr = make_shared<wstring>();

							return httpClient.GetAsync(*authUrlPtr, move(headers))
											 .then([](TResponse &response)
												 {
													 return GetPostContentAsync(move(response));
												 })
											 .then([httpClient, authUrlPtr](wstring &postContent)
												 {
													 unordered_map<wstring, wstring> postHeaders
															 {
																 {
																	 HttpRequestHeaders::Accept,
																	 L"text/html"
																 },
																 {
																	 HttpRequestHeaders::UserAgent,
																	 L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36"
																 },
																 {
																	 HttpRequestHeaders::Connection,
																	 L"close"
																 },
																 {
																	 HttpRequestHeaders::ContentType,
																	 L"application/x-www-form-urlencoded"
																 },
																 {
																	 HttpRequestHeaders::Origin,
																	 L"https://login.wi-fi.ru"
																 },
																 {
																	 HttpRequestHeaders::Referer,
																	 *authUrlPtr
																 }
															 };

													 return httpClient.PostAsync(*authUrlPtr, move(postContent), move(postHeaders));
												 })
											 .then([](TResponse &response) -> AuthResult
												 {
													 return GetStatusCode(move(response)) != 401
																? AuthResult::Success
																: AuthResult::Fail;
												 });
						});

					return checkInternetAvailabilityTask.then([authTask](wstring &authUrl)
						{
							if (authUrl.empty())
							{
								return task_from_result(AuthResult::None);
							}

							return authTask;
						});
				}
				catch (...)
				{
					return task_from_result(AuthResult::Fail);
				}
			}

			virtual bool CanAuth(const std::wstring &connectionName) const NOEXCEPT override
			{
				return connectionName.compare(L"MosMetro_Free") == 0;
			}

			virtual std::wstring GetRegistrationUrl() const NOEXCEPT override
			{
				return GetRegistrationUrlImpl();
			}

		private:
			static std::wstring GetRegistrationUrlImpl() NOEXCEPT
			{
				return L"https://login.wi-fi.ru/am/UI/Login";
			}

			static uint_fast16_t GetStatusCode(TResponse response);

			static Concurrency::task<std::wstring> GetAuthUrlAsync(TResponse response);

			static Concurrency::task<std::wstring> GetPostContentAsync(TResponse response);
		};
	}
}
