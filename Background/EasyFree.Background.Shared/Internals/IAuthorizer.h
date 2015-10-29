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
				virtual Concurrency::task<bool> Authorize(ABI::Windows::Networking::Connectivity::IConnectionProfile* connectionProfile) const NOEXCEPT = 0;
			};
		}
	}
}
