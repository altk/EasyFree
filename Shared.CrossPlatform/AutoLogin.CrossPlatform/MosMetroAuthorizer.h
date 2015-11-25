#pragma once
#include <IAuthorizer.h>
#include <AuthStatus.h>
#include <HttpClient.h>
#include <MTL.h>
#include <chrono>
#include <MosMetroResponseParcer.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

struct HttpSocketClient
{
	static std::string Get(ABI::Windows::Foundation::IUriRuntimeClass* uri)
	{
		using namespace MTL;
		using namespace std;

		HString hostHstring;
		HString queryHstring;
		INT32 port;

		uri->get_Host(&hostHstring);
		uri->get_AbsoluteUri(&queryHstring);
		uri->get_Port(&port);

		wstring hostWstring(hostHstring.GetRawBuffer());
		wstring queryWstring(queryHstring.GetRawBuffer());

		WSADATA wsaData;
		auto ConnectSocket = INVALID_SOCKET;
		addrinfo *result = nullptr,
				hints = {0};

		auto sendbuf = string("GET ").append(string(begin(queryWstring), end(queryWstring))).append(" HTTP1/1\r\n")
									 .append("HOST: ").append(string(begin(hostWstring), end(hostWstring))).append("\r\n\r\n");
		char recvbuf[512];
		int iResult;

		WSAStartup(WINSOCK_VERSION, &wsaData);

		hints.ai_family = AF_UNSPEC ;
		hints.ai_socktype = SOCK_STREAM ;
		hints.ai_protocol = IPPROTO_TCP;

		getaddrinfo(string(begin(hostWstring), end(hostWstring)).data(),
					to_string(port).data(),
					&hints,
					&result);

		ConnectSocket = socket(result->ai_family,
							   result->ai_socktype,
							   result->ai_protocol);

		connect(ConnectSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));

		freeaddrinfo(result);

		send(ConnectSocket, sendbuf.data(), sendbuf.size(), 0);


		shutdown(ConnectSocket, SD_SEND);

		string stringResult;
		do
		{
			iResult = recv(ConnectSocket,
						   recvbuf,
						   extent<decltype(recvbuf)>::value,
						   0);
			stringResult.append(recvbuf, recvbuf + iResult);
		}
		while (iResult > 0);

		closesocket(ConnectSocket);
		WSACleanup();

		return stringResult;
	}
};

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
				using namespace ABI::Windows::Foundation;
				using namespace MTL;
				using namespace std::chrono;
				using namespace std;
				using namespace Concurrency;
				using namespace Resources;

				try
				{
					auto startTime = high_resolution_clock::now();

					const HttpClient<TResponse> httpClient;

					wstring bindUrl = L"http://httpbin.org/status/500";

					ComPtr<IUriRuntimeClassFactory> uriFactory;
					ComPtr<IUriRuntimeClass> uri;

					Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
											   &uriFactory));

					Check(uriFactory->CreateUri(HStringReference(bindUrl.data(), bindUrl.size()).Get(),
												&uri));

					auto authUrl = MosMetroResponseParser::GetAuthUrl(HttpSocketClient::Get(uri.Get()).data());

					OutputDebugStringA("First responce received. Time: ");
					OutputDebugStringA(to_string(duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count()).data());
					OutputDebugStringA("\r\n");

					authUrl = wstring(L"https://login.wi-fi.ru/am/UI/Login?org=mac&service=coa&client_mac=c8-d1-0b-01-24-e1&ForceAuth=true");

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
									 .then([startTime](TResponse response)
										 {
											 OutputDebugStringA("Second responce received. Time: ");
											 OutputDebugStringA(to_string(duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count()).data());
											 OutputDebugStringA("\r\n");

											 return GetPostContentAsync(move(response));
										 })
									 .then([httpClient, authUrl, startTime](wstring postContent)
										 {
											 OutputDebugStringA("PostContent taked. Time: ");
											 OutputDebugStringA(to_string(duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count()).data());
											 OutputDebugStringA("\r\n");

											 return httpClient.PostAsync(move(authUrl), move(postContent));
										 })
									 .then([httpClient, bindUrl](TResponse)
										 {
											 return httpClient.GetAsync(move(bindUrl));
										 })
									 .then([](TResponse response)
										 {
											 return CheckAsync(move(response), 500);
										 })
									 .then([authUrl, startTime](bool checkResult)
										 {
											 OutputDebugStringA("Check completed. Time: ");
											 OutputDebugStringA(to_string(duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count()).data());
											 OutputDebugStringA("\r\n");

											 return checkResult ? wstring(AuthStatus::launchAttributeSuccess) : authUrl;
										 });
				}
				catch (...) { }
				return task_from_result(wstring());
			}

			virtual bool CanAuth(std::wstring connectionName) const NOEXCEPT override
			{
				return true;
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
