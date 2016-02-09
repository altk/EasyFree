#include "pch.h"
#include <MosMetroAuthorizer.h>
#include <MosMetroResponseParcer.h>
#include <HttpHeader.h>
#include <HttpClient.h>
#include <PostData.h>
#include <LicenseChecker.h>
#include <robuffer.h>
#include <windows.web.http.h>
#include <windows.storage.streams.h>
#include <windows.networking.connectivity.h>
#include <MTL.h>

using namespace std;
using namespace Concurrency;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Web::Http::Headers;
using namespace ABI::Windows::Web::Http::Filters;
using namespace ABI::Windows::Web::Http;
using namespace ABI::Windows::Storage::Streams;
using namespace Windows::Storage::Streams;
using namespace MTL;
using namespace AutoLogin::CrossPlatform;

using THttpClient = AutoLogin::CrossPlatform::HttpClient<ComPtr<IHttpResponseMessage>>;

const wstring AcceptHeaderValue = L"text/html";
const wstring UserAgentHeaderValue = L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36";

task<ComPtr<IBuffer>> GetContentAsync(const ComPtr<IHttpResponseMessage> &response)
{
    ComPtr<IHttpContent> httpContent;
    ComPtr<IAsyncOperationWithProgress<IBuffer*, UINT64>> readAsBufferOperation;

    Check(response->get_Content(&httpContent));

    Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

    return GetTask(readAsBufferOperation.Get());
}

HttpStatusCode GetStatusCode(const ComPtr<IHttpResponseMessage> &response)
{
    HttpStatusCode responseStatusCode;
    Check(response->get_StatusCode(&responseStatusCode));

    return responseStatusCode;
}

void PostImpl(THttpClient httpClient,
              ComPtr<IHttpResponseMessage> response,
              task_completion_event<AuthResult> tce,
              uint16_t attempt = 0)
{
    ComPtr<IHttpRequestMessage> request;
    ComPtr<IUriRuntimeClass> uri;
    HString uriValue;

    Check(response->get_RequestMessage(&request));

    Check(request->get_RequestUri(&uri));

    Check(uri->get_AbsoluteUri(&uriValue));

    GetContentAsync(response).then([](ComPtr<IBuffer> buffer)-> PostData
                                 {
                                     ComPtr<IBufferByteAccess> bufferByteAccess;
                                     Check(buffer->QueryInterface<IBufferByteAccess>(&bufferByteAccess));

                                     byte *content;
                                     Check(bufferByteAccess->Buffer(&content));

                                     uint_fast32_t length;
                                     Check(buffer->get_Length(&length));

                                     return MosMetroResponseParser::GetPostData(reinterpret_cast<char*>(content), length);
                                 })
                             .then([httpClient, uriValue](PostData postData)-> task<ComPtr<IHttpResponseMessage>>
                                 {
                                     auto headers = unordered_map<HttpHeader, wstring>
                                             {
                                                 {HttpHeader::Accept, AcceptHeaderValue},
                                                 {HttpHeader::UserAgent, UserAgentHeaderValue},
                                                 {HttpHeader::Referer, wstring(uriValue.GetRawBuffer())}
                                             };

                                     wstring url;
                                     if (postData.getAction().empty())
                                     {
                                         url = wstring(uriValue.GetRawBuffer());
                                     }
                                     else
                                     {
                                         url = postData.getAction();
                                     }

                                     return httpClient.PostAsync(url, headers, make_pair(wstring(L"application/x-www-form-urlencoded"), wstring(postData.getData())));
                                 })
                             .then([httpClient, tce, attempt](ComPtr<IHttpResponseMessage> attemptResponse) -> void
                                 {
                                     auto statusCode = GetStatusCode(attemptResponse);

                                     if (HttpStatusCode_Ok <= statusCode && statusCode < HttpStatusCode_BadRequest)
                                     {
                                         tce.set(AuthResult::Success);
                                     }
                                     else if (attempt > 5)
                                     {
                                         tce.set(AuthResult::Fail);
                                     }
                                     else
                                     {
                                         PostImpl(httpClient, attemptResponse, tce, attempt + 1);
                                     }
                                 });
}

task<AuthResult> MosMetroAuthorizer::AuthAsync() NOEXCEPT
{
    try
    {
        const THttpClient httpClient;

        auto licenseCheckTask = create_task(LicenseChecker::Check);

        return httpClient.GetAsync(L"http://wi-fi.ru")
                         .then([licenseCheckTask](ComPtr<IHttpResponseMessage> response) -> task<ComPtr<IBuffer>>
                             {
                                 const auto statusCode = GetStatusCode(response);

                                 /*if (statusCode == HttpStatusCode_Ok)
                                 {
                                     cancel_current_task();
                                 }*/

                                 return licenseCheckTask.then([response](bool licenceOk) -> task<ComPtr<IBuffer>>
                                     {
                                         if (!licenceOk)
                                         {
                                             cancel_current_task();
                                         }

                                         return GetContentAsync(response);
                                     });
                             })
                         .then([httpClient](ComPtr<IBuffer> buffer) -> task<ComPtr<IHttpResponseMessage>>
                             {
                                 ComPtr<IBufferByteAccess> bufferByteAccess;
                                 Check(buffer->QueryInterface<IBufferByteAccess>(&bufferByteAccess));

                                 byte *content;
                                 Check(bufferByteAccess->Buffer(&content));

                                 uint_fast32_t length;
                                 Check(buffer->get_Length(&length));

                                 auto authUrl = MosMetroResponseParser::GetAuthUrl(reinterpret_cast<char*>(content), length);
                                 if (authUrl.empty())
                                 {
                                     cancel_current_task();
                                 }
                                 
                                 auto headers = unordered_map<HttpHeader, wstring>
                                         {
                                             {HttpHeader::Accept, AcceptHeaderValue},
                                             {HttpHeader::UserAgent, UserAgentHeaderValue}
                                         };

                                 return httpClient.GetAsync(authUrl, headers);
                             })
                         .then([httpClient](ComPtr<IHttpResponseMessage> response) -> task<AuthResult>
                             {
                                 task_completion_event<AuthResult> tce;

                                 PostImpl(httpClient, response, tce);

                                 return task<AuthResult>(tce);
                             })
                         .then([](task<AuthResult> task) -> AuthResult
                             {
                                 try
                                 {
                                     return task.get();
                                 }
                                 catch (const task_canceled &)
                                 {
                                     return AuthResult::None;
                                 }
                                 catch (...)
                                 {
                                     return AuthResult::Fail;
                                 }
                             });
    }
    catch (...)
    {
        return task_from_result(AuthResult::Fail);
    }
}
