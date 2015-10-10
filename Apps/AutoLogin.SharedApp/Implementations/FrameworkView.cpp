#include "pch.h"
#include "FrameworkView.h"
#include <d2d1_1.h>
#include <d3d11_1.h>
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

#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11")

HRESULT FrameworkView::GetRuntimeClassName(HSTRING* result)
{
	*result = HString(L"AutoLogin.FrameworkView").Detach();
	return S_OK;
}

HRESULT FrameworkView::Initialize(ICoreApplicationView* applicationView)
{
	using namespace std;
	using namespace D2D1;

	auto token = make_shared<EventRegistrationToken>();
	auto callback = CreateCallback<ITypedEventHandler<CoreApplicationView*, IActivatedEventArgs*>>([token](ICoreApplicationView* coreApplicationView, IActivatedEventArgs* args)-> HRESULT
																								   {
																									   coreApplicationView->remove_Activated(*token);

																									   ComPtr<ICoreWindow> coreWindow;
																									   coreApplicationView->get_CoreWindow(&coreWindow);
																									   coreWindow->Activate();

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

																									   ComPtr<IDXGISwapChain1> dxgiSwapChain;
																									   DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc = {};
																									   dxgiSwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
																									   dxgiSwapChainDesc.SampleDesc.Count = 1;
																									   dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
																									   dxgiSwapChainDesc.BufferCount = 2;
																									   dxgiFactory->CreateSwapChainForCoreWindow(dxgiDevice.Get(),
																																				 coreWindow.Get(),
																																				 &dxgiSwapChainDesc,
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


																									   return S_OK;
																								   });
	applicationView->add_Activated(callback.Get(), token.get());
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
