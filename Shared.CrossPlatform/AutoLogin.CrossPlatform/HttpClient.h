#pragma once
#include <string>
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

			Concurrency::task<TResponse> GetAsync(std::wstring url) const NOEXCEPT;

			Concurrency::task<TResponse> PostAsync(std::wstring url,
												   std::wstring postContent) const NOEXCEPT;
		private:
			class HttpClientImpl;

			std::shared_ptr<HttpClientImpl> _impl;
		};
	}
}
