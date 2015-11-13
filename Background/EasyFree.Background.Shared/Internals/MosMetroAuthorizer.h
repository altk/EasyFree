#pragma once
#include <AuthStatus.h>

namespace EasyFree
{
	namespace Background
	{
		namespace Internals
		{
			struct MosMetroAuthorizer final 
			{
				static Concurrency::task<EasyFree::Internals::AuthStatus::Enum> Authorize() NOEXCEPT;

				static bool CanAuth(const wchar_t* const connectionName) NOEXCEPT;
			};
		}
	}
}
