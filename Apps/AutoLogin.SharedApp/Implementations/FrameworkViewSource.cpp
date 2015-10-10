#include "pch.h"
#include "FrameworkViewSource.h"
#include "FrameworkView.h"
#include <MTL\Client\ComPtr.h>
#include <MTL\Wrappers\HString.h>

using namespace ABI::Windows::ApplicationModel::Core;
using namespace AutoLogin::Implementations;
using namespace MTL::Client;
using namespace MTL::Wrappers;

HRESULT FrameworkViewSource::GetRuntimeClassName(HSTRING* result)
{
	*result = HString(L"AutoLogin.FrameworkViewSource").Detach();
	return S_OK;
}

HRESULT FrameworkViewSource::CreateView(IFrameworkView** viewProvider)
{
	*viewProvider = ComPtr<IFrameworkView>(new FrameworkView()).Get();
	return S_OK;
}
