#include <pch.h>
#include "LicenseChecker.h"
#include "Base64.h"
#include <sha3.h>
#include <Windows.ApplicationModel.h>
#include <MTL.h>

using namespace AutoLogin::CrossPlatform;

bool LicenseChecker::Check() NOEXCEPT
{
	using namespace std;
	using namespace MTL;
	using namespace CryptoPP;
	using namespace ABI::Windows::ApplicationModel;

	try
	{
		ComPtr<IPackageStatics> packageStatics;
		ComPtr<IPackage> currentPackage;
		ComPtr<IPackageId> packageId;
		HString packageFamilyName;
		HString publisherId;
		PackageVersion packageVersion;

		MTL::Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Package).Get(),
										&packageStatics));

		MTL::Check(packageStatics->get_Current(&currentPackage));

		MTL::Check(currentPackage->get_Id(&packageId));

		MTL::Check(packageId->get_FamilyName(&packageFamilyName));

		MTL::Check(packageId->get_PublisherId(&publisherId));

		MTL::Check(packageId->get_Version(&packageVersion));

		auto identity = wstring(packageFamilyName.GetRawBuffer()).append(publisherId.GetRawBuffer())
			.append(to_wstring(packageVersion.Major))
			.append(L".")
			.append(to_wstring(packageVersion.Minor));
		
		std::array<byte, SHA3_512::DIGESTSIZE> digest;

		SHA3_512().CalculateDigest(digest.data(),
								   reinterpret_cast<const byte*>(identity.data()),
								   sizeof(wchar_t) * identity.size());

		return strcmp(Base64::Encode(reinterpret_cast<unsigned const char*>(digest.data()), digest.size()).data(),
					  "Fks2ScR41cRSMSv00BX65xjS02kM6zXv8AuEY/fFpDig8Bh6+Pdl9n6ymZEkiaMYVobBKHwSXVhFscbnr5ntxQ==") == 0;
	}
	catch (...)
	{
		return false;
	}
}
