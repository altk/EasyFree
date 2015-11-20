#pragma once
#include <IAuthorizer.h>
#include <AuthStatus.h>
#include <HttpClient.h>
#include <MosMetroResponseParcer.h>
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

				wstring bindUrl = L"http://httpbin.org/status/500";

				try
				{
					const HttpClient<TResponse> httpClient;

					return httpClient.GetAsync(bindUrl)
									 .then([](TResponse response)
										 {
											 return GetContentAsync(move(response));
										 })
									 .then([](string content)
										 {
											 return MosMetroResponseParser::GetFormUrl(move(content));
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

											 return httpClient.GetAsync(authUrl)
															  .then([](TResponse response)
																  {
																	  return GetContentAsync(move(response));
																  })
															  .then([](string content)
																  {
																	  return MosMetroResponseParser::GetPostString(move(content));
																  })
															  .then([httpClient, authUrl](wstring postContent)
																  {
																	  return httpClient.PostAsync(move(authUrl), move(postContent));
																  })
															  .then([](TResponse response)
																  {
																	  return CheckAsync(move(response), 500);
																  })
															  .then([authUrl](bool checkResult)
																  {
																	  return checkResult ? wstring(AuthStatus::launchAttributeSuccess) : authUrl;
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

			static Concurrency::task<bool> CheckAsync(TResponse response, uint_fast16_t statusCode) NOEXCEPT;

			static Concurrency::task<std::string> GetContentAsync(TResponse response) NOEXCEPT;
		};
	}
}
