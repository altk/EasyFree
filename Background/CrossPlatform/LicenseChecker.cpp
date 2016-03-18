#include <pch.h>
#include "LicenseChecker.h"
#include "Base64.h"
#include <sha256.h>
#include <Windows.ApplicationModel.h>
#include <MTL.h>

using namespace AutoLogin::CrossPlatform;

bool LicenseChecker::Check() NOEXCEPT
{
    using namespace std;
    using namespace MTL;
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

        array<uint8_t, SHA256_BLOCK_SIZE> digest;
        SHA256_CTX ctx;

        sha256_init(&ctx);
        sha256_update(&ctx, reinterpret_cast<const byte*>(identity.data()), sizeof(wchar_t) * identity.size());
        sha256_final(&ctx, digest.data());

        return Base64::Encode(reinterpret_cast<unsigned const char* const>(digest.data()), digest.size()).compare("eD8MU7AO235vNPYDpCMXsu8LBopwquNLnW1qYjk8egY=") == 0;
    }
    catch (...)
    {
        return false;
    }
}
