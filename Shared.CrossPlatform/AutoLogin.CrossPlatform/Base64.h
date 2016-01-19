#pragma once
#include <string>
#include <macro.h>

namespace AutoLogin
{
	namespace CrossPlatform
	{
		class Base64 final
		{
		public:
			static std::string Encode(unsigned char const *bytes,
									  unsigned int length) NOEXCEPT
			{
				using namespace std;

				string result;
				auto i = 0;
				auto j = 0;
				unsigned char char_array_3[3];
				unsigned char char_array_4[4];

				char baseChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

				while (length--)
				{
					char_array_3[i++] = *(bytes++);
					if (i == 3)
					{
						char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
						char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
						char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
						char_array_4[3] = char_array_3[2] & 0x3f;

						for (i = 0; (i < 4); i++) result += baseChars[char_array_4[i]];
						i = 0;
					}
				}

				if (i)
				{
					for (j = i; j < 3; j++) char_array_3[j] = '\0';

					char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
					char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
					char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
					char_array_4[3] = char_array_3[2] & 0x3f;

					for (j = 0; (j < i + 1); j++) result += baseChars[char_array_4[j]];

					while ((i++ < 3)) result += '=';
				}

				return result;
			}
		};
	}
}
