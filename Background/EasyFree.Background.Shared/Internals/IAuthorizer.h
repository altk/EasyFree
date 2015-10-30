#pragma once
#include <windows.networking.connectivity.h>
#include <ppltasks.h>
#include <macro.h>

namespace EasyFree
{
	namespace Background
	{
		namespace Internals
		{
			struct
					NOVTABLE IAuthorizer abstract
			{
				virtual Concurrency::task<bool> Authorize() const NOEXCEPT = 0;

				virtual bool CanAuth(ABI::Windows::Networking::Connectivity::IConnectionProfile* connectionProfile) const NOEXCEPT = 0;
			};
		}
	}
}
