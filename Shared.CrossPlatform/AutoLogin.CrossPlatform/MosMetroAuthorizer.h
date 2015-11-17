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
		class MosMetroAuthorizer final : IAuthorizer
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

			virtual Concurrency::task<AuthStatus::Enum> AuthAsync() NOEXCEPT override
			{
				using namespace std;
				using namespace Concurrency;

				wstring bindUrl = L"http://httpbin.org/status/200";

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
									 .then([](string authUrl)
										 {
											 return wstring(begin(authUrl), end(authUrl));
										 })
									 .then([httpClient, bindUrl](wstring authUrl) -> task<AuthStatus::Enum>
										 {
											 if (authUrl.empty())
											 {
												 return task_from_result(AuthStatus::None);
											 }

											 if (authUrl == GetAuthUrlImpl())
											 {
												 return task_from_result(AuthStatus::Unauthorized);
											 }

											 return httpClient.GetAsync(authUrl)
															  .then([](TResponse response)
																  {
																	  return GetContentAsync(move(response));
																  })
															  .then([](string content)
																  {
																	  return MosMetroResponseParser::GetPostString(content);
																  })
															  .then([](string postContent)
																  {
																	  return wstring(begin(postContent), end(postContent));
																  })
															  .then([httpClient, authUrl](wstring postContent)
																  {
																	  return httpClient.PostAsync(move(authUrl), move(postContent));
																  })
															  .then([httpClient, bindUrl](TResponse response)
																  {
																	  return CheckAsync(move(response));
																  })
															  .then([](bool checkResult)
																  {
																	  return checkResult ? AuthStatus::Success : AuthStatus::Fail;
																  });
										 });
				}
				catch (...) { }
				return task_from_result(AuthStatus::None);
			}

			virtual bool CanAuth(std::wstring connectionName) const NOEXCEPT override
			{
				return connectionName.compare(L"MosMetro_Free") == 0;
			}

			virtual std::wstring GetAuthUrl() const NOEXCEPT override
			{
				return GetAuthUrlImpl();
			}

		private:
			static std::wstring GetAuthUrlImpl() NOEXCEPT
			{
				return L"https://login.wi-fi.ru/am/UI/Login";
			}

			static Concurrency::task<bool> CheckAsync(TResponse response) NOEXCEPT;

			static Concurrency::task<std::string> GetContentAsync(TResponse response) NOEXCEPT;
		};
	}
}
