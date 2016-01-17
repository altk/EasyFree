#pragma once
#include <string>
#include <memory>
#include <macro.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		class SettingsProvider final
		{
		public:
			SettingsProvider();

			~SettingsProvider() NOEXCEPT;

			SettingsProvider(const SettingsProvider &other) NOEXCEPT
				: _impl(other._impl) {}

			SettingsProvider(SettingsProvider &&other) NOEXCEPT
				: _impl(move(other._impl)) {}

			SettingsProvider& operator=(const SettingsProvider &other) NOEXCEPT
			{
				if (this != &other)
				{
					_impl = other._impl;
				}
				return *this;
			}

			SettingsProvider& operator=(SettingsProvider &&other) NOEXCEPT
			{
				if (this != &other)
				{
					_impl = move(other._impl);
				}
				return *this;
			}

			std::wstring Get(const std::wstring &key) const NOEXCEPT;

			bool Set(const std::wstring &key, const std::wstring &value) const NOEXCEPT;
		private:
			class SettingsProviderImpl;

			std::shared_ptr<SettingsProviderImpl> _impl;
		};
	}
}
