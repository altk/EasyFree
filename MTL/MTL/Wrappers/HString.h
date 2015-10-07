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

			static Pointer Invalid() noexcept
			{
				return nullptr;
			}

			static void Close(Pointer value) noexcept
			{
				VERIFY_SUCCEEDED(WindowsDeleteString(value));
			}
		};

		class HString final : public Handle<HStringTraits>
		{
		public:
			explicit HString(Pointer pointer = HStringTraits::Invalid()) noexcept
				: Handle<HStringTraits>(pointer)
			{
			}

			HString(const wchar_t* const string, unsigned const length) noexcept
			{
				VERIFY_SUCCEEDED(WindowsCreateString(string, length, GetAddressOf()));
			}

			template <unsigned Length>
			explicit HString(const wchar_t(&string)[Length]) noexcept
				: HString(string, Length-1)
			{
			}

			HString(const HString& other) noexcept
			{
				VERIFY_SUCCEEDED(WindowsDuplicateString(other.Get(), GetAddressOf()));
			}

			HString(HString&&) noexcept = default;

			HString& operator=(const HString& other) noexcept
			{
				if (this != &other)
				{
					Close();
					VERIFY_SUCCEEDED(WindowsDuplicateString(other.Get(), GetAddressOf()));
				}
				return *this;
			}

			HString& operator=(HString&&) noexcept = default;

			HString Substring(unsigned start) const noexcept
			{
				HString result;
				VERIFY_SUCCEEDED(WindowsSubstring(Get(), start, result.GetAddressOf()));
				return result;
			}

			const wchar_t* GetRawBuffer() const noexcept
			{
				return WindowsGetStringRawBuffer(Get(), nullptr);
			}

			const wchar_t* GetRawBuffer(unsigned* length) const noexcept
			{
				return WindowsGetStringRawBuffer(Get(), length);
			}

			unsigned Size() const noexcept
			{
				return WindowsGetStringLen(Get());
			}

			bool Empty() const noexcept
			{
				return 0 == WindowsIsStringEmpty(Get());
			}
		};
	}
}