#include "pch.h"
#include "FrameworkView.h"
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <MTL\Client\Async.h>
#include <MTL\Client\ComPtr.h>
#include <MTL\Wrappers\HString.h>
#include <MTL\Wrappers\HStringReference.h>

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::UI::Core;
using namespace AutoLogin::Implementations;
using namespace MTL::Client;
using namespace MTL::Wrappers;
using namespace std;
using namespace D2D1;

#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11")

HRESULT FrameworkView::GetRuntimeClassName(HSTRING* result)
{
	*result = HString(L"AutoLogin.FrameworkView").Detach();
	return S_OK;
}

HRESULT FrameworkView::Initialize(ICoreApplicationView* applicationView)
{
	auto token = make_shared<EventRegistrationToken>();
	auto callback = CreateCallback<ITypedEventHandler<CoreApplicationView*, IActivatedEventArgs*>>([token](ICoreApplicationView* coreApplicationView, IActivatedEventArgs* args)-> HRESULT
																								   {
																									   coreApplicationView->remove_Activated(*token);

																									   ComPtr<ICoreWindow> coreWindow;
																									   coreApplicationView->get_CoreWindow(&coreWindow);
																									   coreWindow->Activate();
																									   /*
																									   

*/
																									   return S_OK;
																								   });
	applicationView->add_Activated(callback.Get(), token.get());
	return S_OK;
}

HRESULT FrameworkView::SetWindow(ICoreWindow* window)
{
	_coreWindow.Reset(window);

	/*ComPtr<ICoreCursorFactory> coreCursorFactory;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Core_CoreCursor).Get(),
						 &coreCursorFactory);

	ComPtr<ICoreCursor> coreCursor;
	coreCursorFactory->CreateCursor(CoreCursorType_Arrow,
									0,
									&coreCursor);

	_coreWindow->put_PointerCursor(coreCursor.Get());*/

	return S_OK;
}

HRESULT FrameworkView::Load(HSTRING entryPoint)
{
	return S_OK;
}

HRESULT FrameworkView::Run()
{
	ComPtr<ICoreDispatcher> dispatcher;
	_coreWindow->get_Dispatcher(&dispatcher);

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

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
	swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
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

	while (true)
	{
		dispatcher->ProcessEvents(CoreProcessEventsOption_ProcessAllIfPresent);

		d2dDeviceContext->BeginDraw();
		D2D1_RECT_F rect = {};
		rect.left = 0;
		rect.top = 0;
		rect.right = 100;
		rect.bottom = 100;

		ComPtr<ID2D1SolidColorBrush> brush;
		d2dDeviceContext->CreateSolidColorBrush(ColorF(ColorF::Red),
												&brush);

		d2dDeviceContext->DrawRectangle(rect, brush.Get());
		d2dDeviceContext->EndDraw();

		dxgiSwapChain->Present(1, 0);
	}


	return S_OK;
}

HRESULT FrameworkView::Uninitialize()
{
	return S_OK;
}
