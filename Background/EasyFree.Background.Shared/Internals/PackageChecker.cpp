#include <pch.h>
#include "PackageCkecker.h"
#include "Base64.h"
#include <Windows.ApplicationModel.h>
#include <windows.security.cryptography.h>
#include <windows.security.cryptography.core.h>
#include <MTL.h>

using namespace EasyFree::Background::Internals;

MTL::HString PackageChecker::GetPackageIdentity()
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
	
	auto versionString = to_wstring(packageVersion.Major) + L"." + to_wstring(packageVersion.Minor);
	
	return packageFamilyName + publisherId + HString(versionString.data(), versionString.size());
}

bool PackageChecker::CheckCurrentPackage() NOEXCEPT
{
	using namespace MTL;
	using namespace ABI::Windows::Security::Cryptography::Core;
	using namespace ABI::Windows::Security::Cryptography;
	using namespace ABI::Windows::Storage::Streams;

	try
	{
		ComPtr<IHashAlgorithmProviderStatics> hashAlgorithmProviderStatics;
		ComPtr<IHashAlgorithmNamesStatics> hashAlgorithmNamesStatics;
		ComPtr<IHashAlgorithmProvider> sha512HashAlgorithmProvider;
		ComPtr<ICryptographicBufferStatics> cryptographicBufferStatics;
		ComPtr<IBuffer> packageFullNameBuffer;
		ComPtr<IBuffer> packageFullNameHashBuffer;
		HString sha512Name;
		HString currentPackageHashString;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmProvider).Get(),
								   &hashAlgorithmProviderStatics));

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmNames).Get(),
								   &hashAlgorithmNamesStatics));

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(),
								   &cryptographicBufferStatics));

		Check(hashAlgorithmNamesStatics->get_Sha512(&sha512Name));

		Check(hashAlgorithmProviderStatics->OpenAlgorithm(sha512Name.Get(),
														  &sha512HashAlgorithmProvider));

		Check(cryptographicBufferStatics->ConvertStringToBinary(GetPackageIdentity().Get(),
																BinaryStringEncoding_Utf16LE,
																&packageFullNameBuffer));

		Check(sha512HashAlgorithmProvider->HashData(packageFullNameBuffer.Get(),
													&packageFullNameHashBuffer));

		Check(cryptographicBufferStatics->ConvertBinaryToString(BinaryStringEncoding_Utf16LE,
																packageFullNameHashBuffer.Get(),
																&currentPackageHashString));

		return strcmp(Base64::Encode(reinterpret_cast<unsigned const char*>(currentPackageHashString.GetRawBuffer()), sizeof(wchar_t) * currentPackageHashString.Size()).data(),
					  "JW2tnHFTgbB3vA8CYHMERpRdCj0lUkRGU+l/2n7yBougkya1rTpBnNFjVc3k3fuKs6hxTqatywcUJbvyK1oaPw==") == 0;
	}
	catch (...)
	{
		return false;
	}
}
