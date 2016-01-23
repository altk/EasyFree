#include <pch.h>
#include <SettingsProvider.h>
#include <windows.storage.h>
#include <MTL.h>

using namespace std;
using namespace AutoLogin::CrossPlatform;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace MTL;

class SettingsProvider::SettingsProviderImpl final
{
public:
	SettingsProviderImpl()
	{
		ComPtr<IApplicationDataStatics> applicationDataStatics;
		ComPtr<IApplicationData> applicationData;
		ComPtr<IApplicationDataContainer> applicationDataContainer;
		ComPtr<IPropertySet> propertySet;

		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(),
								   &applicationDataStatics));

		Check(applicationDataStatics->get_Current(&applicationData));

		Check(applicationData->get_LocalSettings(&applicationDataContainer));

		Check(applicationDataContainer->get_Values(&propertySet));

		Check(propertySet.As(&_propertyMap));
	}

	wstring Get(const wstring &key) const NOEXCEPT
	{
		wstring result;

		try
		{
			ComPtr<IInspectable> inspectable;
			Check(_propertyMap->Lookup(HStringReference(key).Get(),
									   &inspectable));

			if (inspectable)
			{
				ComPtr<IPropertyValue> propertyValue;
				HString value;

				Check(inspectable.As(&propertyValue));

				Check(propertyValue->GetString(&value));

				result.append(value.GetRawBuffer());
			}
		}
		catch (...) {}

		return result;
	}

	bool Set(const wstring &key, const wstring &value) NOEXCEPT
	{
		try
		{
			boolean isReplaced;
			ComPtr<IPropertyValueStatics> propertyValueStatics;
			ComPtr<IPropertyValue> propertyValue;

			Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
									   &propertyValueStatics));

			Check(propertyValueStatics->CreateString(HString(value).Get(),
													 &propertyValue));

			Check(_propertyMap->Insert(HStringReference(key).Get(),
									   propertyValue.Get(),
									   &isReplaced));
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	bool Delete(const wstring &key) NOEXCEPT
	{
		try
		{
			Check(_propertyMap->Remove(HStringReference(key).Get()));
			
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	bool Contains(const wstring &key) const
	{
		boolean found;
		Check(_propertyMap->HasKey(HStringReference(key).Get(),
								   &found));

		return found;
	}

private:
	ComPtr<IMap<HSTRING, IInspectable*>> _propertyMap;
};

SettingsProvider::SettingsProvider()
	: _impl(make_shared<SettingsProviderImpl>()) {}

SettingsProvider::~SettingsProvider() NOEXCEPT {}

wstring SettingsProvider::Get(const wstring &key) const NOEXCEPT
{
	return _impl->Get(key);
}

bool SettingsProvider::Set(const wstring &key, const wstring &value) NOEXCEPT
{
	return _impl->Set(key, value);
}

bool SettingsProvider::Delete(const std::wstring &key) NOEXCEPT
{
	return _impl->Delete(key);
}

bool SettingsProvider::HasKey(const std::wstring &key) const
{
	return _impl->Contains(key);
}
