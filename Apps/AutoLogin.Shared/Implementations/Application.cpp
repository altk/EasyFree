#include "pch.h"
#include "Application.h"
#include <memory>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <windows.graphics.display.h>
#include <windows.applicationmodel.background.h>
#include <windows.foundation.collections.h>
#include <MTL\Wrappers\HString.h>
#include <MTL\Wrappers\HStringReference.h>
#include <MTL\Client\Async.h>

using namespace AutoLogin::Implementations;

HRESULT Application::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
	using namespace MTL::Wrappers;

	*className = HString(L"AutoLogin.Application").Detach();
	return S_OK;
}

HRESULT Application::CreateView(ABI::Windows::ApplicationModel::Core::IFrameworkView** viewProvider) NOEXCEPT
{
	*viewProvider = this;
	return S_OK;
}

HRESULT Application::Initialize(ABI::Windows::ApplicationModel::Core::ICoreApplicationView* applicationView) NOEXCEPT
{
	using namespace std;
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::ApplicationModel::Core;
	using namespace ABI::Windows::ApplicationModel::Activation;
	using namespace ABI::Windows::UI::Core;
	using namespace MTL::Client;

	auto token = make_shared<EventRegistrationToken>();
	auto callback = CreateCallback<ITypedEventHandler<CoreApplicationView*, IActivatedEventArgs*>>([token](ICoreApplicationView* coreApplicationView, IActivatedEventArgs* args)-> HRESULT
																								   {
																									   coreApplicationView->remove_Activated(*token);
																									   ComPtr<ICoreWindow> coreWindow;
																									   coreApplicationView->get_CoreWindow(&coreWindow);
																									   coreWindow->Activate();
																									   return S_OK;
																								   });

	applicationView->add_Activated(callback.Get(), token.get());
	return S_OK;
}

HRESULT Application::SetWindow(ABI::Windows::UI::Core::ICoreWindow* window) NOEXCEPT
{
	using namespace std;
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::ApplicationModel::Core;
	using namespace ABI::Windows::ApplicationModel::Activation;
	using namespace ABI::Windows::UI::Core;
	using namespace MTL::Client;

	_coreWindow.Attach(window);

	InitContext();

	auto visibilityChangedToken = make_shared<EventRegistrationToken>();
	auto visibilityChangedCallback = CreateCallback<ITypedEventHandler<CoreWindow*, VisibilityChangedEventArgs*>>([this, visibilityChangedToken](ICoreWindow* coreWindow, IVisibilityChangedEventArgs* args)-> HRESULT
																												  {
																													  coreWindow->remove_VisibilityChanged(*visibilityChangedToken);

																													  Draw();

																													  return S_OK;
																												  });
	window->add_VisibilityChanged(visibilityChangedCallback.Get(), visibilityChangedToken.get());

	return S_OK;
}

HRESULT Application::Load(HSTRING) NOEXCEPT
{
	return S_OK;
}

HRESULT Application::Run() NOEXCEPT
{
	using namespace ABI::Windows::UI::Core;
	using namespace MTL::Client;

	RegisterBackgroundTask();

	ComPtr<ICoreDispatcher> coreDispatcher;
	_coreWindow->get_Dispatcher(&coreDispatcher);
	coreDispatcher->ProcessEvents(CoreProcessEventsOption_ProcessUntilQuit);

	return S_OK;
}

HRESULT Application::Uninitialize() NOEXCEPT
{
	_coreWindow.Release();
	return S_OK;
}

void Application::InitContext() NOEXCEPT
{
	using namespace D2D1;
	using namespace ABI::Windows::ApplicationModel::Core;
	using namespace ABI::Windows::UI::Core;
	using namespace ABI::Windows::Graphics::Display;
	using namespace ABI::Windows::Foundation;
	using namespace MTL::Client;
	using namespace MTL::Wrappers;

	ComPtr<IDisplayInformationStatics> displayInformationStatics;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
						 &displayInformationStatics);

	ComPtr<IDisplayInformation> displayInformation;
	displayInformationStatics->GetForCurrentView(&displayInformation);

	FLOAT dpiX,
		  dpiY;

	displayInformation->get_RawDpiX(&dpiX);
	displayInformation->get_RawDpiY(&dpiY);

	ComPtr<ID2D1Factory1> factory;
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
					  D2D1_FACTORY_OPTIONS{},
					  static_cast<ID2D1Factory1**>(&factory));

	ComPtr<ID3D11Device> device;
	D3D11CreateDevice(nullptr,
					  D3D_DRIVER_TYPE_HARDWARE,
					  nullptr,
					  D3D11_CREATE_DEVICE_BGRA_SUPPORT,
					  nullptr, 0,
					  D3D11_SDK_VERSION,
					  device.GetAddressOf(),
					  nullptr,
					  nullptr);

	ComPtr<IDXGIDevice> dxgiDevice;
	device.As(&dxgiDevice);

	ComPtr<IDXGIAdapter> dxgiAdapter;
	dxgiDevice->GetAdapter(&dxgiAdapter);

	ComPtr<IDXGIFactory2> dxgiFactory;
	dxgiAdapter->GetParent(__uuidof(dxgiFactory),
						   &dxgiFactory);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
	swapChainDesc.Stereo = false;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.Flags = 0;
	swapChainDesc.Width = 0;
	swapChainDesc.Height = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	dxgiFactory->CreateSwapChainForCoreWindow(dxgiDevice.Get(),
											  _coreWindow.Get(),
											  &swapChainDesc,
											  nullptr,
											  &_swapChain);

	ComPtr<ID2D1Device> d2dDevice;
	factory->CreateDevice(dxgiDevice.Get(),
						  &d2dDevice);

	d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
								   &_deviceContext);

	ComPtr<IDXGISurface> dxgiSurface;
	_swapChain->GetBuffer(0, __uuidof(dxgiSurface), &dxgiSurface);

	auto bitmapProperties = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
											  PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
											  dpiX,
											  dpiY);

	ComPtr<ID2D1Bitmap1> bitmap;
	_deviceContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(),
												bitmapProperties,
												bitmap.GetAddressOf());
	_deviceContext->SetTarget(bitmap.Get());
	_deviceContext->SetDpi(dpiX, dpiY);
}

void Application::Draw() NOEXCEPT
{
	using namespace D2D1;
	using namespace MTL::Client;

	D2D1_RECT_F rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.right = 100;
	rect.bottom = 100;

	ComPtr<ID2D1SolidColorBrush> brush;
	_deviceContext->CreateSolidColorBrush(ColorF(ColorF::Red),
										  &brush);

	_deviceContext->BeginDraw();
	_deviceContext->DrawRectangle(rect, brush.Get());
	_deviceContext->EndDraw();

	_swapChain->Present(1, 0);
}

void Application::RegisterBackgroundTask() NOEXCEPT
{
	using namespace ABI::Windows::ApplicationModel::Background;
	using namespace ABI::Windows::Foundation::Collections;
	using namespace ABI::Windows::Foundation;
	using namespace MTL::Client;
	using namespace MTL::Wrappers;

	ComPtr<IBackgroundTaskRegistrationStatics> backgroundTaskRegistrationStatics;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundTaskRegistration).Get(),
						 &backgroundTaskRegistrationStatics);

	ComPtr<IMapView<GUID, IBackgroundTaskRegistration*>> taskRegistrations;
	backgroundTaskRegistrationStatics->get_AllTasks(&taskRegistrations);

	ComPtr<IIterable<IKeyValuePair<GUID, IBackgroundTaskRegistration*>*>> registrationsIterable;
	taskRegistrations.As(&registrationsIterable);

	ComPtr<IIterator<IKeyValuePair<GUID, IBackgroundTaskRegistration*>*>> registrationsIterator;
	registrationsIterable->First(&registrationsIterator);

	boolean hasCurrent;
	registrationsIterator->get_HasCurrent(&hasCurrent);

	while (hasCurrent)
	{
		ComPtr<IKeyValuePair<GUID, IBackgroundTaskRegistration*>> current;
		registrationsIterator->get_Current(&current);

		ComPtr<IBackgroundTaskRegistration> taskRegistration;
		current->get_Value(&taskRegistration);

		taskRegistration->Unregister(true);

		registrationsIterator->MoveNext(&hasCurrent);
	}

	ComPtr<IBackgroundTaskBuilder> backgroundTaskBuilder;
	ActivateInstance<IBackgroundTaskBuilder>(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundTaskBuilder).Get(),
											 &backgroundTaskBuilder);

	backgroundTaskBuilder->put_Name(HString(L"AutoLogin Wi-Fi Background task").Detach());
	backgroundTaskBuilder->put_TaskEntryPoint(HString(L"AutoLogin.Background.LoginTask").Detach());

	ComPtr<ISystemTriggerFactory> systemTriggerActivationFactory;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_SystemTrigger).Get(),
						 &systemTriggerActivationFactory);

	ComPtr<ISystemTrigger> systemTrigger;
	systemTriggerActivationFactory->Create(SystemTriggerType_NetworkStateChange,
										   false,
										   &systemTrigger);

	ComPtr<IBackgroundTrigger> backgroundTrigger;
	systemTrigger.As(&backgroundTrigger);

	backgroundTaskBuilder->SetTrigger(backgroundTrigger.Get());

	ComPtr<IBackgroundTaskRegistration> taskRegistration;
	backgroundTaskBuilder->Register(&taskRegistration);
}

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) NOEXCEPT
{
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::ApplicationModel::Core;
	using namespace ABI::Windows::ApplicationModel;
	using namespace MTL::Client;
	using namespace MTL::Wrappers;

	RoInitialize(RO_INIT_MULTITHREADED);

	ComPtr<ICoreApplication> coreApplication;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
						 &coreApplication);

	Application app;
	coreApplication->Run(&app);
}
