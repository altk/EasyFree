#pragma once
#include <windows.foundation.h>
#include <MTL.h>

namespace AutoLogin
{
	namespace Windows
	{
		class UriUtilities final
		{
		public:
			UriUtilities()
			{
				using namespace MTL;
				using namespace ABI::Windows::Foundation;

				Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
										   &_uriStatics));
			}

			UriUtilities(const UriUtilities& other) NOEXCEPT
				: _uriStatics(other._uriStatics) {}

			UriUtilities(UriUtilities&& other) NOEXCEPT
				: _uriStatics(std::move(other._uriStatics)) {}

			UriUtilities& operator=(const UriUtilities& other) NOEXCEPT
			{
				if (this != &other)
				{
					_uriStatics = other._uriStatics;
				}
				return *this;
			}

			UriUtilities& operator=(UriUtilities&& other) NOEXCEPT
			{
				if (this != &other)
				{
					_uriStatics = std::move(other._uriStatics);
				}
				return *this;
			}

			void Escape(HSTRING source,
						HSTRING* destination) const
			{
				using namespace MTL;

				Check(_uriStatics->EscapeComponent(source, destination));
			}
			
			MTL::HString Escape(HSTRING source) const
			{
				using namespace MTL;

				HString result;
				Check(_uriStatics->EscapeComponent(source, &result));
				return result;
			}

			MTL::HString Escape(const std::wstring& source) const
			{
				using namespace MTL;
			
				return Escape(HStringReference(source).Get());
			}

			void Unescape(HSTRING source,
						  HSTRING* destination) const
			{
				using namespace MTL;

				Check(_uriStatics->UnescapeComponent(source, destination));
			}

			MTL::HString Unescape(HSTRING source) const
			{
				using namespace MTL;

				HString result;
				Check(_uriStatics->UnescapeComponent(source, &result));
				return result;
			}

			MTL::HString Unescape(std::wstring source) const
			{
				using namespace MTL;

				return Unescape(HStringReference(source).Get());
			}

		private:
			MTL::ComPtr<ABI::Windows::Foundation::IUriEscapeStatics> _uriStatics;
		};
	}
}
