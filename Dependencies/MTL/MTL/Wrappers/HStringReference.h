#pragma once
#include <winstring.h>
#include <macro.h>

namespace MTL
{
	namespace Wrappers
	{
		class HStringReference final 
		{
		public:
			HStringReference(const wchar_t* const value, unsigned length) NOEXCEPT
			{
				VERIFY_SUCCEEDED(WindowsCreateStringReference(value, length, &_stringHeader, &_string));
			}

			template<unsigned Length>
			explicit HStringReference(const wchar_t(&string)[Length]) NOEXCEPT
				: HStringReference(string, Length - 1)
			{
			}

			HStringReference(const HStringReference&) = delete;

			HStringReference& operator=(const HStringReference&) = delete;
			
			void * operator new(std::size_t) = delete;

			void * operator new[](std::size_t) = delete;
			
			void operator delete(void*) = delete;

			void operator delete[](void*) = delete;

			HSTRING Get() const NOEXCEPT
			{
				return _string;
			}

		private:
			HSTRING _string;
			HSTRING_HEADER _stringHeader;
		};
	}
}