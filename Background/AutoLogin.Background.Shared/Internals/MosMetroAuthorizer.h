#pragma once
#include <string>
#include <AuthStatus.h>

namespace AutoLogin
{
	namespace Background
	{
		namespace Internals
		{
			struct MosMetroAuthorizer final 
			{
				static Concurrency::task<AutoLogin::Internals::AuthStatus::Enum> AuthAsync() NOEXCEPT;

				static bool CanAuth(const wchar_t* const connectionName) NOEXCEPT;

				static std::string GetAuthUrl() NOEXCEPT;
			};
		}
	}
}
