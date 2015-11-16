#pragma once
#include <macro.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		struct LicenseChecker final
		{
			static bool Check() NOEXCEPT;
		};
	}
}
