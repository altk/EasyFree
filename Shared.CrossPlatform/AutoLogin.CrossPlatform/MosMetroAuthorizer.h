#pragma once
#include <memory>
#include <IAuthorizer.h>

namespace AutoLogin
{
    namespace CrossPlatform
    {
        class MosMetroAuthorizer final : public IAuthorizer
        {
        public:
            virtual Concurrency::task<AuthResult> AuthAsync() NOEXCEPT override;

            virtual bool CanAuth(const std::wstring &connectionName) const NOEXCEPT override
            {
                return true;
                return connectionName.compare(L"MosMetro_Free") == 0;
            }

            virtual std::wstring GetRegistrationUrl() const NOEXCEPT override
            {
                return GetRegistrationUrlImpl();
            }

            //{
            //    using namespace std;
            //    using namespace Concurrency;
            //    using namespace Resources;

            //    try
            //    {
            //        const HttpClient<TResponse> httpClient;

            //        auto licenseCheckTask = create_task(LicenseChecker::Check);

            //        auto getAuthUrlTask = httpClient.GetAsync(L"http://wi-fi.ru")
            //                                        .then([](TResponse &response)
            //                                            {
            //                                                return GetAuthUrlAsync(move(response));
            //                                            });

            //        auto authTask = getAuthUrlTask.then([httpClient, licenseCheckTask](wstring &authUrl) mutable -> task<AuthResult>
            //            {
            //                if (authUrl.empty())
            //                {
            //                    return task_from_result(AuthResult::None);
            //                }

            //                if (authUrl == GetRegistrationUrlImpl())
            //                {
            //                    return task_from_result(AuthResult::Unregistered);
            //                }

            //                auto pAuthUrl = make_shared<wstring>(move(authUrl));

            //                auto pHeaders = make_shared<unordered_map<HttpHeader, wstring>>();
            //                pHeaders->emplace(HttpHeader::Accept, L"text/html");
            //                pHeaders->emplace(HttpHeader::UserAgent, L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36");

            //                auto postContentTask = httpClient.GetAsync(*pAuthUrl, *pHeaders)
            //                                                 .then([](TResponse &response)
            //                                                     {
            //                                                         return GetPostContentAsync(move(response));
            //                                                     });

            //                return licenseCheckTask.then([httpClient, pAuthUrl, pHeaders, postContentTask](bool licenceOk)-> task<AuthResult>
            //                    {
            //                        if (!licenceOk)
            //                        {
            //                            return task_from_result(AuthResult::Unlicensed);
            //                        }

            //                        return postContentTask.then([httpClient, pAuthUrl, pHeaders](wstring &postContent)
            //                                                  {
            //                                                      pHeaders->emplace(HttpHeader::Referer, *pAuthUrl);

            //                                                      return httpClient.PostAsync(*pAuthUrl,
            //                                                                                  *pHeaders,
            //                                                                                  make_pair(wstring(L"application/x-www-form-urlencoded"), move(postContent)));
            //                                                  })
            //                                              .then([](TResponse &response) -> AuthResult
            //                                                  {
            //                                                      return GetStatusCode(move(response)) != 401
            //                                                                 ? AuthResult::Success
            //                                                                 : AuthResult::Fail;
            //                                                  });
            //                    });
            //            });

            //        auto resultTask = checkInternetAvailabilityTask.then([authTask](wstring &authUrl)
            //            {
            //                if (authUrl.empty())
            //                {
            //                    return task_from_result(AuthResult::None);
            //                }

            //                return authTask;
            //            });

            //        //Если адрес авторизации не был сохранён
            //        if (savedAuthUrl.empty())
            //        {
            //            return resultTask;
            //        }

            //        return resultTask.then([settingsProvider, pAuthUrlKey](AuthResult authResult) mutable
            //            {
            //                try
            //                {
            //                    wstring tryCountKey = L"TryCount";
            //                    auto countString = settingsProvider.Get(tryCountKey);
            //                    switch (authResult)
            //                    {
            //                        case AuthResult::Fail:
            //                            if (countString.empty())
            //                            {
            //                                settingsProvider.Set(tryCountKey, L"1");
            //                            }
            //                            else
            //                            {
            //                                auto count = stoi(settingsProvider.Get(tryCountKey));
            //                                if (++count > 5)
            //                                {
            //                                    settingsProvider.Delete(tryCountKey);
            //                                    settingsProvider.Delete(*pAuthUrlKey);
            //                                }
            //                                else
            //                                {
            //                                    settingsProvider.Set(tryCountKey, to_wstring(count));
            //                                }
            //                            }
            //                            break;
            //                        case AuthResult::Success:
            //                            if (!countString.empty())
            //                            {
            //                                settingsProvider.Delete(tryCountKey);
            //                            }
            //                            break;
            //                    }
            //                }
            //                catch (...) {}

            //                return authResult;
            //            });
            //    }
            //    catch (...)
            //    {
            //        return task_from_result(AuthResult::Fail);
            //    }
            //}

        private:
            static std::wstring GetRegistrationUrlImpl() NOEXCEPT
            {
                return L"https://login.wi-fi.ru/am/UI/Login";
            }



            /*static uint_fast16_t GetStatusCode(TResponse response);

            static Concurrency::task<std::wstring> GetAuthUrlAsync(TResponse response);

            static Concurrency::task<PostData> GetPostContentAsync(TResponse response);*/
        };
    }
}
