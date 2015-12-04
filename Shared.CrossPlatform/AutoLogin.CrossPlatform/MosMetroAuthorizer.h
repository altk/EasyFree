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
			MosMetroAuthorizer() NOEXCEPT {}

			MosMetroAuthorizer(const MosMetroAuthorizer&) NOEXCEPT {}

			MosMetroAuthorizer(MosMetroAuthorizer&&) {}

			MosMetroAuthorizer& operator=(const MosMetroAuthorizer&) NOEXCEPT
			{
				return *this;
			}

			MosMetroAuthorizer& operator=(MosMetroAuthorizer&&) NOEXCEPT
			{
				return *this;
			}

			virtual Concurrency::task<std::wstring> AuthAsync() NOEXCEPT override
			{
				using namespace std;
				using namespace Concurrency;
				using namespace Resources;

				try
				{
					const HttpClient<TResponse> httpClient;
					const SettingsProvider settingsProvider;
					auto authSettingKeyPtr = make_shared<wstring>(L"AuthKeySettings");

					task<wstring> getAuthUrlTask;

					auto savedUrl = settingsProvider.Get(*authSettingKeyPtr);
					if (savedUrl.empty())
					{
						getAuthUrlTask = httpClient.GetAsync(L"http://wi-fi.ru")
												   .then([](TResponse& response)
													   {
														   return GetAuthUrlAsync(move(response));
													   })
												   .then([settingsProvider, authSettingKeyPtr](wstring& authUrl)
													   {
														   settingsProvider.Set(*authSettingKeyPtr, authUrl);
														   return authUrl;
													   });
					}
					else
					{
						getAuthUrlTask = task_from_result(savedUrl);
					}

					return getAuthUrlTask.then([httpClient](wstring& authUrl) -> task<wstring>
						{
							if (authUrl.empty())
							{
								return task_from_result(wstring());
							}

							auto registrationUrl = GetRegistrationUrlImpl();
							if (authUrl == registrationUrl)
							{
								return task_from_result(move(registrationUrl));
							}

							unordered_map<wstring, wstring> getHeaders
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

							auto authUrlPtr = make_shared<wstring>(move(authUrl));

							return httpClient.GetAsync(*authUrlPtr,
													   move(getHeaders))
											 .then([](TResponse& response)
												 {
													 return GetPostContentAsync(move(response));
												 })
											 .then([httpClient, authUrlPtr](wstring& postContent)
												 {
													 unordered_map<wstring, wstring> postHeaders
															 {
																 {
																	 HttpRequestHeaders::Accept,
																	 L"text/html"
																 },
																 {
																	 HttpRequestHeaders::Origin,
																	 L"https://login.wi-fi.ru"
																 },
																 {
																	 HttpRequestHeaders::UserAgent,
																	 L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36"
																 },
																 {
																	 HttpRequestHeaders::ContentType,
																	 L"application/x-www-form-urlencoded"
																 },
																 {
																	 HttpRequestHeaders::Referer,
																	 *authUrlPtr
																 },
																 {
																	 HttpRequestHeaders::Connection,
																	 L"close"
																 }
															 };

													 return httpClient.PostAsync(*authUrlPtr,
																				 move(postContent),
																				 move(postHeaders));
												 })
											 .then([httpClient, authUrlPtr](TResponse& response) -> wstring
												 {
													 return GetStatusCode(move(response)) != 401
																? AuthStatus::launchAttributeSuccess
																: *authUrlPtr;
												 });
						});
				}
				catch (...) { }
				return task_from_result(wstring());
			}

			virtual bool CanAuth(const std::wstring& connectionName) const NOEXCEPT override
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
