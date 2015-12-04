#include <pch.h>
#include <SettingsProvider.h>
#include <windows.storage.h>
#include <MTL.h>

using namespace std;
using namespace AutoLogin::CrossPlatform;
using namespace ABI::Windows::Foundation::Collections;
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

	wstring Get(const wstring& key) const NOEXCEPT
	{
		wstring result;

		try
		{
			HString value;
			Check(_propertyMap->Lookup(HStringReference(key).Get(),
									   &value));
			
			if (value)
			{
				result.append(value.GetRawBuffer());
			}
		}
		catch (...) {}

		return result;
	}

	bool Set(const wstring& key,
			 const wstring& value) const NOEXCEPT
	{
		try
		{
			boolean isReplaced;
			Check(_propertyMap->Insert(HStringReference(key).Get(),
									   HStringReference(value).Get(),
									   &isReplaced));
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

private:
	ComPtr<IMap<HSTRING, HSTRING>> _propertyMap;
};

SettingsProvider::SettingsProvider()
	: _impl(make_shared<SettingsProviderImpl>()) {}

SettingsProvider::~SettingsProvider() NOEXCEPT {}

wstring SettingsProvider::Get(const wstring& key) const NOEXCEPT
{
	return _impl->Get(key);
}

bool SettingsProvider::Set(const wstring& key,
						   const wstring& value) const NOEXCEPT
{
	return _impl->Set(key, value);
}
