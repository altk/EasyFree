#pragma once
#include <string>
#include <unordered_map>
#include <tuple>
#include <memory>
#include <ppltasks.h>
#include <macro.h>
#include <HttpHeader.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		template <typename TResponse>
		class HttpClient
		{
		public:
			using TUrl = std::wstring;
			using THeaders = std::unordered_map<HttpHeader, std::wstring>;
			using TContent = std::wstring;
			using TContentType = std::wstring;
			using TPostContent = std::pair<TContentType, TContent>;

			HttpClient() NOEXCEPT;

			~HttpClient() NOEXCEPT;

			HttpClient(const HttpClient &other) NOEXCEPT
				: _impl(other._impl) {}

			HttpClient(HttpClient &&other) NOEXCEPT
				: _impl(std::move(other._impl)) { }

			HttpClient& operator=(const HttpClient &other) NOEXCEPT
			{
				if (this != &other)
				{
					_impl = other._impl;
				}
				return *this;
			}

			HttpClient& operator=(HttpClient &&other) NOEXCEPT
			{
				if (this != &other)
				{
					_impl = std::move(other._impl);
				}
				return *this;
			}

			Concurrency::task<TResponse> GetAsync(const TUrl &url,
												  const THeaders &headers = THeaders()) const;

			Concurrency::task<TResponse> PostAsync(const TUrl &url,
												   const THeaders &headers = THeaders(),
												   const TPostContent &postContent = TPostContent()) const;
		private:
			class HttpClientImpl;

			std::shared_ptr<HttpClientImpl> _impl;
		};
	}
}
