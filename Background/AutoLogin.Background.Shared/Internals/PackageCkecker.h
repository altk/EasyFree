#pragma once
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
			};
		}
	}
}
