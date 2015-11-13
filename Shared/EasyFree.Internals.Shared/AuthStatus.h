#pragma once

namespace EasyFree
{
	namespace Internals
	{
		struct AuthStatus
		{
			enum Enum
			{
				Success,
				Fail,
				Unauthorized,
				Unlicensed,
				None
			};

			static const wchar_t* const launchAttributeSuccess;
			
			static const wchar_t* const launchAttributeFail;
			
			static const wchar_t* const launchAttributeUnlicensed;

			static const wchar_t* const launchAttributeScheme;
		};
	}
}