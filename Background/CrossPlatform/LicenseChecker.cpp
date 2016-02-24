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

        wostringstream identityStream;
        identityStream << packageFamilyName.GetRawBuffer()
                << publisherId.GetRawBuffer()
                << packageVersion.Major
                << packageVersion.Minor;
        
        auto identity = identityStream.str();

        array<byte, SHA3_512::DIGESTSIZE> digest;
        SHA3_512().CalculateDigest(digest.data(),
                                   reinterpret_cast<const byte*>(identity.data()),
                                   sizeof(wchar_t) * identity.size());

        return Base64::Encode(reinterpret_cast<unsigned const char*>(digest.data()), digest.size()).compare("BBFX8TyLiZr8vG0fVK+wHcSnMgsBEA5+dWwFPvW/J2XpKudWRXfnD3NBs4Jftb9Xdffl+qWQfDqF2iBTLqqMlw==") == 0;
    }
    catch (...)
    {
        return false;
    }
}
