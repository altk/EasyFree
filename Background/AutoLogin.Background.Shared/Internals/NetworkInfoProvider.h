#pragma once
#include <windows.networking.connectivity.h>
#include <MTL.h>

namespace AutoLogin
{
	namespace Background
	{
		namespace Internals
		{
			struct NetworkInfoProvider final
			{
				static MTL::ComPtr<ABI::Windows::Networking::Connectivity::IConnectionProfile> GetNetworkConnectionProfile() NOEXCEPT
				{
					using namespace MTL;
					using namespace ABI::Windows::Networking::Connectivity;

					ComPtr<INetworkInformationStatics> networkInformationStatics;
					ComPtr<IConnectionProfile> connectionProfile;

					try
					{
						Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(),
												   &networkInformationStatics));

						Check(networkInformationStatics->GetInternetConnectionProfile(&connectionProfile));
					}
					catch (const ComException&) {}

					return connectionProfile;
				}
			};
		}
	}
}
