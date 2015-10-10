#include "pch.h"
#include "FrameworkView.h"
#include <MTL\Client\ComPtr.h>
#include <MTL\Wrappers\HString.h>
#include <MTL\Client\Async.h>

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::UI::Core;
using namespace AutoLogin::Implementations;
using namespace MTL::Client;
using namespace MTL::Wrappers;

HRESULT FrameworkView::GetRuntimeClassName(HSTRING* result)
{
	*result = HString(L"AutoLogin.FrameworkView").Detach();
	return S_OK;
}

HRESULT FrameworkView::Initialize(ICoreApplicationView* applicationView)
{
	auto callback = CreateCallback<ITypedEventHandler<CoreApplicationView*, IActivatedEventArgs*>>([](ICoreApplicationView* coreApplicationView, IActivatedEventArgs* args)-> HRESULT
																								   {
																									   ComPtr<ICoreWindow> coreWindow;
																									   coreApplicationView->get_CoreWindow(&coreWindow);
																									   coreWindow->Activate();
																									   return S_OK;
																								   });
	EventRegistrationToken token;
	applicationView->add_Activated(callback.Get(), &token);
	return S_OK;
}

HRESULT FrameworkView::SetWindow(ICoreWindow* window)
{
	_pCoreWindow.Reset(window);
	return S_OK;
}

HRESULT FrameworkView::Load(HSTRING entryPoint)
{
	return S_OK;
}

HRESULT FrameworkView::Run()
{

	ComPtr<ICoreDispatcher> dispatcher;
	_pCoreWindow->get_Dispatcher(&dispatcher);
	dispatcher->ProcessEvents(CoreProcessEventsOption_ProcessUntilQuit);

	return S_OK;
}

HRESULT FrameworkView::Uninitialize()
{
	return S_OK;
}
