#pragma once
#include <string>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		struct HttpRequestHeaders final
		{
			static const std::wstring Accept;
			static const std::wstring ContentType;
			static const std::wstring Origin;
			static const std::wstring Referer;
			static const std::wstring UserAgent;
		};
	}
}
