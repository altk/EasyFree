#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <ppltasks.h>
#include <macro.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		template <typename TResponse>
		class HttpClient
		{
		public:
			using TUrl = std::wstring;
			using TContent = std::wstring;
			using THeaders = std::unordered_map<std::wstring, std::wstring>;

			HttpClient() NOEXCEPT;

			~HttpClient() NOEXCEPT;

			HttpClient(const HttpClient& other) NOEXCEPT
				: _impl(other._impl) {}

			HttpClient(HttpClient&& other) NOEXCEPT
				: _impl(std::move(other._impl)) { }

			HttpClient& operator=(const HttpClient& other) NOEXCEPT
			{
				if (this != &other)
				{
					_impl = other._impl;
				}
				return *this;
			}

			HttpClient& operator=(HttpClient&& other) NOEXCEPT
			{
				if (this != &other)
				{
					_impl = std::move(other._impl);
				}
				return *this;
			}

			Concurrency::task<TResponse> GetAsync(TUrl url, 
												  THeaders headers = THeaders()) const;

			Concurrency::task<TResponse> PostAsync(TUrl url, 
												   TContent postContent, 
												   THeaders headers = THeaders()) const;
		private:
			class HttpClientImpl;

			std::shared_ptr<HttpClientImpl> _impl;
		};
	}
}
