#pragma once
#include <winstring.h>
#include <MTL\Wrappers\Handle.h>
#include <macro.h>

namespace MTL
{
	namespace Wrappers
	{		
		struct HStringTraits final
		{
			using Pointer = HSTRING;

			static Pointer Invalid() NOEXCEPT
			{
				return nullptr;
			}

			static void Close(Pointer value) NOEXCEPT
			{
				VERIFY_SUCCEEDED(WindowsDeleteString(value));
			}
		};

		class HString final : public Handle<HStringTraits>
		{
		public:
			explicit HString(Pointer pointer = HStringTraits::Invalid()) NOEXCEPT
				: Handle<HStringTraits>(pointer)
			{
			}

			HString(const wchar_t* const string, unsigned const length) NOEXCEPT
			{
				VERIFY_SUCCEEDED(WindowsCreateString(string, length, GetAddressOf()));
			}

			template <unsigned Length>
			explicit HString(const wchar_t(&string)[Length]) NOEXCEPT
				: HString(string, Length-1)
			{
			}

			HString(const HString& other) NOEXCEPT
			{
				VERIFY_SUCCEEDED(WindowsDuplicateString(other.Get(), GetAddressOf()));
			}

			HString(HString&&) NOEXCEPT = default;

			HString& operator=(const HString& other) NOEXCEPT
			{
				if (this != &other)
				{
					Close();
					VERIFY_SUCCEEDED(WindowsDuplicateString(other.Get(), GetAddressOf()));
				}
				return *this;
			}

			HString& operator=(HString&&) NOEXCEPT = default;

			HString Substring(unsigned start) const NOEXCEPT
			{
				HString result;
				VERIFY_SUCCEEDED(WindowsSubstring(Get(), start, result.GetAddressOf()));
				return result;
			}

			const wchar_t* GetRawBuffer() const NOEXCEPT
			{
				return WindowsGetStringRawBuffer(Get(), nullptr);
			}

			const wchar_t* GetRawBuffer(unsigned* length) const NOEXCEPT
			{
				return WindowsGetStringRawBuffer(Get(), length);
			}

			unsigned Size() const NOEXCEPT
			{
				return WindowsGetStringLen(Get());
			}

			bool Empty() const NOEXCEPT
			{
				return 0 == WindowsIsStringEmpty(Get());
			}
		};
	}
}