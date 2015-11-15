#pragma once
#include <string>
#include <macro.h>

namespace AutoLogin
{
	namespace Background
	{
		namespace Internals
		{
			class PackageChecker final
			{
			public:
				static bool CheckCurrentPackage() NOEXCEPT;
			private:
				static std::wstring GetPackageIdentity();
			};
		}
	}
}
