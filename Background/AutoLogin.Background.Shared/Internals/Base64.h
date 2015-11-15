#pragma once
#include <string>
#include <macro.h>

namespace AutoLogin
{
	namespace Background
	{
		namespace Internals
		{
			class Base64 final
			{
			public:
				static std::string Encode(unsigned char const* bytes_to_encode, unsigned int in_len) NOEXCEPT
				{
					std::string ret;
					auto i = 0;
					auto j = 0;
					unsigned char char_array_3[3];
					unsigned char char_array_4[4];

					while (in_len--)
					{
						char_array_3[i++] = *(bytes_to_encode++);
						if (i == 3)
						{
							char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
							char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
							char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
							char_array_4[3] = char_array_3[2] & 0x3f;

							for (i = 0; (i < 4); i++) ret += GetBase64Chars()[char_array_4[i]];
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

						for (j = 0; (j < i + 1); j++) ret += GetBase64Chars()[char_array_4[j]];

						while ((i++ < 3)) ret += '=';
					}

					return ret;
				}

				static std::string Decode(std::string const& encoded_string) NOEXCEPT
				{
					auto in_len = encoded_string.size();
					auto i = 0;
					auto j = 0;
					auto in_ = 0;
					unsigned char char_array_4[4], char_array_3[3];
					std::string ret;

					while (in_len-- && (encoded_string[in_] != '=') && IsBase64(encoded_string[in_]))
					{
						char_array_4[i++] = encoded_string[in_];
						in_++;
						if (i == 4)
						{
							for (i = 0; i < 4; i++) char_array_4[i] = GetBase64Chars().find(char_array_4[i]);

							char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
							char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
							char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

							for (i = 0; (i < 3); i++) ret += char_array_3[i];
							i = 0;
						}
					}

					if (i)
					{
						for (j = i; j < 4; j++) char_array_4[j] = 0;

						for (j = 0; j < 4; j++) char_array_4[j] = GetBase64Chars().find(char_array_4[j]);

						char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
						char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
						char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

						for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
					}

					return ret;
				}

			private:
				static const std::string& GetBase64Chars() NOEXCEPT
				{
					static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
					return chars;
				}

				static bool IsBase64(unsigned char c) NOEXCEPT
				{
					return (isalnum(c) || (c == '+') || (c == '/'));
				}
			};
		}
	}
}