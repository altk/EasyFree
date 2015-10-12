#include "pch.h"
#include "Application.h"
#include <memory>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <MTL\Wrappers\HString.h>
#include <MTL\Wrappers\HStringReference.h>
#include <MTL\Client\Async.h>

using namespace AutoLogin;

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
	_coreWindow.Attach(window);
	return S_OK;
}

HRESULT Application::Load(HSTRING) NOEXCEPT
{
	return S_OK;
}

HRESULT Application::Run() NOEXCEPT
{
	using namespace D2D1;
	using namespace ABI::Windows::ApplicationModel::Core;
	using namespace ABI::Windows::UI::Core;
	using namespace MTL::Client;

	ComPtr<ICoreDispatcher> coreDispatcher;
	_coreWindow->get_Dispatcher(&coreDispatcher);

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
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.Flags = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	ComPtr<IDXGISwapChain1> dxgiSwapChain;
	dxgiFactory->CreateSwapChainForCoreWindow(dxgiDevice.Get(),
											  _coreWindow.Get(),
											  &swapChainDesc,
											  nullptr,
											  &dxgiSwapChain);

	ComPtr<ID2D1Device> d2dDevice;
	factory->CreateDevice(dxgiDevice.Get(),
						  &d2dDevice);

	ComPtr<ID2D1DeviceContext> d2dDeviceContext;
	d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
								   &d2dDeviceContext);

	ComPtr<IDXGISurface> dxgiSurface;
	dxgiSwapChain->GetBuffer(0, __uuidof(dxgiSurface), &dxgiSurface);

	auto bitmapProperties = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
											  PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

	ComPtr<ID2D1Bitmap1> bitmap;
	d2dDeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(),
												  bitmapProperties,
												  bitmap.GetAddressOf());
	d2dDeviceContext->SetTarget(bitmap.Get());

	d2dDeviceContext->SetDpi(332.0f, 332.0f);

	D2D1_RECT_F rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.right = 100;
	rect.bottom = 100;

	ComPtr<ID2D1SolidColorBrush> brush;
	d2dDeviceContext->CreateSolidColorBrush(ColorF(ColorF::Red),
											&brush);

	while (true)
	{
		coreDispatcher->ProcessEvents(CoreProcessEventsOption_ProcessAllIfPresent);
		
		d2dDeviceContext->BeginDraw();
		d2dDeviceContext->DrawRectangle(rect, brush.Get());
		d2dDeviceContext->EndDraw();

		dxgiSwapChain->Present(1, 0);
	}

	return S_OK;
}

HRESULT Application::Uninitialize() NOEXCEPT
{
	_coreWindow.Release();
	return S_OK;
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
