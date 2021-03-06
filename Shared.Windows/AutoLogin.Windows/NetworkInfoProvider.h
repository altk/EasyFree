#pragma once
#include <windows.networking.connectivity.h>
#include <MTL.h>

namespace AutoLogin
{
	namespace Windows
	{
		struct NetworkInfoProvider final
		{
			static MTL::ComPtr<ABI::Windows::Networking::Connectivity::IConnectionProfile> GetNetworkConnectionProfile()
			{
				using namespace MTL;
				using namespace ABI::Windows::Networking::Connectivity;

				ComPtr<INetworkInformationStatics> networkInformationStatics;
				ComPtr<IConnectionProfile> connectionProfile;

				Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(),
											&networkInformationStatics));

				Check(networkInformationStatics->GetInternetConnectionProfile(&connectionProfile));

				return connectionProfile;
			}
		};
	}
}
