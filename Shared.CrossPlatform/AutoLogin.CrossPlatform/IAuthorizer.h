#pragma once
#include <string>
#include <ppltasks.h>
#include <AuthStatus.h>
#include <macro.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		struct NOVTABLE IAuthorizer abstract
		{
			virtual Concurrency::task<Resources::AuthStatus::Enum> AuthAsync() NOEXCEPT = 0;

			virtual bool CanAuth(std::wstring connectionName) const NOEXCEPT = 0;

			virtual std::wstring GetAuthUrl() const NOEXCEPT = 0;
		};
	}
}
