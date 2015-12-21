#pragma once
#include <string>

namespace AutoLogin
{
	namespace Resources
	{
		struct Labels final
		{
			static const std::wstring Title;

			static const std::wstring SuccessDescription;

			static const std::wstring FailDescription;

			static const std::wstring RegisterBackgroundTaskFail;

			static const std::wstring RegisterBackgroundTaskDenied;
			
			static const std::wstring AuthSuccess;

			static const std::wstring AuthFail;

			static const std::wstring RegistrationNeed;

			static const std::wstring Unlicensed;
		};
	}
}
