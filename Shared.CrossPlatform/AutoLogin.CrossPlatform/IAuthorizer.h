#pragma once
#include <string>
#include <ppltasks.h>
#include <macro.h>
#include <AuthResult.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		struct NOVTABLE IAuthorizer abstract
		{
			virtual Concurrency::task<AuthResult> AuthAsync() NOEXCEPT = 0;

			virtual bool CanAuth(const std::wstring &connectionName) const NOEXCEPT = 0;

			virtual std::wstring GetRegistrationUrl() const NOEXCEPT = 0;
		};
	}
}
