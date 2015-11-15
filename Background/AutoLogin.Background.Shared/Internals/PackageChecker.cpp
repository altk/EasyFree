#include <pch.h>
#include "PackageCkecker.h"
#include "Base64.h"
#include <sha3.h>
#include <Windows.ApplicationModel.h>
#include <MTL.h>

using namespace AutoLogin::Background::Internals;

std::wstring PackageChecker::GetPackageIdentity()
{
	using namespace std;
	using namespace MTL;
	using namespace ABI::Windows::ApplicationModel;

	ComPtr<IPackageStatics> packageStatics;
	ComPtr<IPackage> currentPackage;
	ComPtr<IPackageId> packageId;
	HString packageFamilyName;
	HString publisherId;
	PackageVersion packageVersion;

	Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Package).Get(),
							   &packageStatics));

	Check(packageStatics->get_Current(&currentPackage));

	Check(currentPackage->get_Id(&packageId));

	Check(packageId->get_FamilyName(&packageFamilyName));

	Check(packageId->get_PublisherId(&publisherId));

	Check(packageId->get_Version(&packageVersion));

	return wstring(packageFamilyName.GetRawBuffer()).append(publisherId.GetRawBuffer())
													.append(to_wstring(packageVersion.Major))
													.append(L".")
													.append(to_wstring(packageVersion.Minor));
}

bool PackageChecker::CheckCurrentPackage() NOEXCEPT
{
	using namespace MTL;
	using namespace CryptoPP;

	try
	{
		auto identity = GetPackageIdentity();
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
