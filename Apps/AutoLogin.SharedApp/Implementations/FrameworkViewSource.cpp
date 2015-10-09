#include "pch.h"
#include "FrameworkViewSource.h"
#include "FrameworkView.h"

using namespace ABI::Windows::ApplicationModel::Core;
using namespace AutoLogin::Implementations;

HRESULT FrameworkViewSource::GetRuntimeClassName(HSTRING* result)
{
	return S_OK;
}

HRESULT FrameworkViewSource::CreateView(IFrameworkView** viewProvider)
{
	*viewProvider = new FrameworkView();
	return S_OK;
}
