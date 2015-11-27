#pragma once
#include <IAuthorizer.h>
#include <AuthStatus.h>
#include <HttpClient.h>
#include <MTL.h>

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

					wstring bindUrl = L"http://httpbin.org/status/500";

					return httpClient.GetAsync(bindUrl, vector<tuple<wstring, wstring>>())
									 .then([](TResponse response)
										 {
											 return GetAuthUrlAsync(move(response));
										 })
									 .then([httpClient, bindUrl](wstring authUrl) -> task<wstring>
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

											 vector<tuple<wstring, wstring>> getHeaders
													 {
														 make_tuple(L"Accept", L"text/html"),
														 make_tuple(L"User-Agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36")
													 };

											 return httpClient.GetAsync(authUrl,
																		move(getHeaders))
															  .then([](TResponse response)
																  {
																	  return GetPostContentAsync(move(response));
																  })
															  .then([httpClient, authUrl](wstring postContent)
																  {
																	  vector<tuple<wstring, wstring>> postHeaders
																			  {
																				  make_tuple(L"Accept", L"text/html"),
																				  make_tuple(L"Origin", L"https://login.wi-fi.ru"),
																				  make_tuple(L"User-Agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36"),
																				  make_tuple(L"Content-Type", L"application/x-www-form-urlencoded"),
																				  make_tuple(L"Referer", authUrl)
																			  };

																	  return httpClient.PostAsync(move(authUrl),
																								  move(postHeaders),
																								  move(postContent));
																  })
															  .then([httpClient, bindUrl](TResponse)
																  {
																	  return httpClient.GetAsync(move(bindUrl), vector<tuple<wstring, wstring>>());
																  })
															  .then([](TResponse response)
																  {
																	  return CheckAsync(move(response), 500);
																  })
															  .then([authUrl](bool checkResult)
																  {
																	  return checkResult ? wstring(AuthStatus::launchAttributeSuccess) : move(authUrl);
																  });
										 });
				}
				catch (...) { }
				return task_from_result(wstring());
			}

			virtual bool CanAuth(std::wstring connectionName) const NOEXCEPT override
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

			static Concurrency::task<bool> CheckAsync(TResponse response,
													  uint_fast16_t statusCode) NOEXCEPT;

			static Concurrency::task<std::wstring> GetAuthUrlAsync(TResponse response) NOEXCEPT;

			static Concurrency::task<std::wstring> GetPostContentAsync(TResponse response) NOEXCEPT;
		};
	}
}
