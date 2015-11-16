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
			virtual Concurrency::task<AuthStatus::Enum> AuthAsync() const NOEXCEPT = 0;

			virtual bool CanAuth(const wchar_t* const connectionName) const NOEXCEPT = 0;

			virtual std::string GetAuthUrl() const NOEXCEPT = 0;
		};
	}
}
