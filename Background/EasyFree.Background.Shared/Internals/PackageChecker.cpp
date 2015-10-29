#include <pch.h>
#include "PackageCkecker.h"
#include "Base64.h"
#include <Windows.ApplicationModel.h>
#include <windows.security.cryptography.h>
#include <windows.security.cryptography.core.h>
#include <MTL.h>

using namespace EasyFree::Background::Internals;

MTL::HString PackageChecker::GetPackageFullName()
{
	using namespace MTL;
	using namespace ABI::Windows::ApplicationModel;

	ComPtr<IPackageStatics> packageStatics;
	ComPtr<IPackage> currentPackage;
	ComPtr<IPackageId> packageId;
	HString fullName;

	Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Package).Get(),
							   &packageStatics));

	Check(packageStatics->get_Current(&currentPackage));

	Check(currentPackage->get_Id(&packageId));

	Check(packageId->get_FullName(&fullName));

	return fullName;
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

		Check(cryptographicBufferStatics->ConvertStringToBinary(GetPackageFullName().Get(),
																BinaryStringEncoding_Utf16LE,
																&packageFullNameBuffer));

		Check(sha512HashAlgorithmProvider->HashData(packageFullNameBuffer.Get(),
													&packageFullNameHashBuffer));

		Check(cryptographicBufferStatics->ConvertBinaryToString(BinaryStringEncoding_Utf16LE,
																packageFullNameHashBuffer.Get(),
																&currentPackageHashString));

		return strcmp(Base64::Encode(reinterpret_cast<unsigned const char*>(currentPackageHashString.GetRawBuffer()), sizeof(wchar_t) * currentPackageHashString.Size()).data(),
					  "1n4SXBHZzohjo+h75K6eXOUood9eKJ1fUt+Vt+VrYTXvTQQVTshgaYF1k5V08hJ2BpZ7hZbQMT8ts2T4rRSbPQ==") == 0;
	}
	catch (...)
	{
		return false;
	}
}
