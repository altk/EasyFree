#include "pch.h"
#include "FrameworkView.h"
#include <MTL/Client/ComPtr.h>

using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::UI::Core;
using namespace AutoLogin::Implementations;
using namespace MTL::Client;

HRESULT FrameworkView::GetRuntimeClassName(HSTRING* result)
{
	return S_OK;
}

HRESULT FrameworkView::Initialize(ICoreApplicationView* applicationView)
{
	return S_OK;
}

HRESULT FrameworkView::SetWindow(ICoreWindow* window)
{
	_pCoreWindow = window;
	return S_OK;
}

HRESULT FrameworkView::Load(HSTRING entryPoint)
{
	return S_OK;
}

HRESULT FrameworkView::Run()
{
	ComPtr<ICoreDispatcher> dispatcher;
	_pCoreWindow->Activate();
	_pCoreWindow->get_Dispatcher(&dispatcher);
	dispatcher->ProcessEvents(CoreProcessEventsOption_ProcessUntilQuit);
	return S_OK;
}

HRESULT FrameworkView::Uninitialize()
{
	return S_OK;
}