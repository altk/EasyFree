#pragma once
#include <macro.h>

namespace EasyFree
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
				static MTL::HString GetPackageIdentity();
			};
		}
	}
}
