#pragma once
#include <string>
#include <IAuthorizer.h>
#include <AuthStatus.h>
#include <macro.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		struct MosMetroAuthorizer final : IAuthorizer
		{
			virtual Concurrency::task<AuthStatus::Enum> AuthAsync() const NOEXCEPT override;

			virtual bool CanAuth(const wchar_t* const connectionName) const NOEXCEPT override;

			virtual std::string GetAuthUrl() const NOEXCEPT override;
		};
	}
}
