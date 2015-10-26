#include "pch.h"
#include "Application.h"
#include <roapi.h>
#include <memory>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite.h>
#include <windows.graphics.display.h>
#include <windows.applicationmodel.background.h>
#include <windows.foundation.collections.h>
#include <MTL.h>

using namespace AutoLogin::Implementations;

HRESULT Application::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
	using namespace MTL;

	try
	{
		*className = HString(RuntimeClass_AutoLogin_Application).Detach();
		return S_OK;
	}
	catch (const ComException& exception)
	{
		return exception.GetResult();
	}
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
	using namespace MTL;

	auto token = make_shared<EventRegistrationToken>();
	auto callback = CreateCallback<ITypedEventHandler<CoreApplicationView*, IActivatedEventArgs*>>(
		[token]
		(ICoreApplicationView* coreApplicationView, IActivatedEventArgs* args)->
		HRESULT
		{
			try
			{
				Check(coreApplicationView->remove_Activated(*token));

				ComPtr<ICoreWindow> coreWindow;
				Check(coreApplicationView->get_CoreWindow(&coreWindow));
				Check(coreWindow->Activate());

				return S_OK;
			}
			catch (const ComException& comException)
			{
				return comException.GetResult();
			}
		});

	try
	{
		Check(applicationView->add_Activated(callback.Get(),
											 token.get()));

		return S_OK;
	}
	catch (const ComException& comException)
	{
		return comException.GetResult();
	}
}

HRESULT Application::SetWindow(ABI::Windows::UI::Core::ICoreWindow* window) NOEXCEPT
{
	using namespace std;
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::ApplicationModel::Core;
	using namespace ABI::Windows::ApplicationModel::Activation;
	using namespace ABI::Windows::UI::Core;
	using namespace MTL;

	_coreWindow.Attach(window);

	auto visibilityChangedCallback = CreateCallback<ITypedEventHandler<CoreWindow*, VisibilityChangedEventArgs*>>(
		[this]
		(ICoreWindow* coreWindow, IVisibilityChangedEventArgs* args)->
		HRESULT
		{
			try
			{
				boolean isVisible;
				Check(args->get_Visible(&isVisible));

				if (!isVisible)
				{
					_deviceContext.Release();
					_swapChain.Release();
					_dwriteFactory.Release();
				}
				if (!_deviceContext)
				{
					InitContext();
				}

				Draw();

				return S_OK;
			}
			catch (const ComException& comException)
			{
				return comException.GetResult();
			}
		});

	try
	{
		EventRegistrationToken tempToken;
		Check(window->add_VisibilityChanged(visibilityChangedCallback.Get(),
											&tempToken));

		return S_OK;
	}
	catch (const ComException& comException)
	{
		return S_OK;
	}
}

HRESULT Application::Load(HSTRING) NOEXCEPT
{
	return S_OK;
}

HRESULT Application::Run() NOEXCEPT
{
	using namespace ABI::Windows::UI::Core;
	using namespace MTL;

	RegisterBackgroundTask();

	try
	{
		ComPtr<ICoreDispatcher> coreDispatcher;
		Check(_coreWindow->get_Dispatcher(&coreDispatcher));
		Check(coreDispatcher->ProcessEvents(CoreProcessEventsOption_ProcessUntilQuit));

		return S_OK;
	}
	catch (const ComException& comException)
	{
		return comException.GetResult();
	}
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
	using namespace MTL;

	try
	{
		ComPtr<IDisplayInformationStatics> displayInformationStatics;
		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
								   &displayInformationStatics));

		ComPtr<IDisplayInformation> displayInformation;
		Check(displayInformationStatics->GetForCurrentView(&displayInformation));

		FLOAT dpiX,
			  dpiY;

		Check(displayInformation->get_RawDpiX(&dpiX));
		Check(displayInformation->get_RawDpiY(&dpiY));

		ComPtr<ID2D1Factory1> factory;
		Check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
								D2D1_FACTORY_OPTIONS{},
								static_cast<ID2D1Factory1**>(&factory)));

		ComPtr<ID3D11Device> device;
		Check(D3D11CreateDevice(nullptr,
								D3D_DRIVER_TYPE_HARDWARE,
								nullptr,
								D3D11_CREATE_DEVICE_BGRA_SUPPORT,
								nullptr, 0,
								D3D11_SDK_VERSION,
								device.GetAddressOf(),
								nullptr,
								nullptr));

		ComPtr<IDXGIDevice> dxgiDevice;
		Check(device.As(&dxgiDevice));

		ComPtr<IDXGIAdapter> dxgiAdapter;
		Check(dxgiDevice->GetAdapter(&dxgiAdapter));

		ComPtr<IDXGIFactory2> dxgiFactory;
		Check(dxgiAdapter->GetParent(__uuidof(dxgiFactory),
									 &dxgiFactory));

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

		Check(dxgiFactory->CreateSwapChainForCoreWindow(dxgiDevice.Get(),
														_coreWindow.Get(),
														&swapChainDesc,
														nullptr,
														&_swapChain));

		ComPtr<ID2D1Device> d2dDevice;
		Check(factory->CreateDevice(dxgiDevice.Get(),
									&d2dDevice));

		Check(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
											 &_deviceContext));

		ComPtr<IDXGISurface> dxgiSurface;
		Check(_swapChain->GetBuffer(0, __uuidof(dxgiSurface), &dxgiSurface));

		auto bitmapProperties = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
												  PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
												  dpiX,
												  dpiY);

		ComPtr<ID2D1Bitmap1> bitmap;
		Check(_deviceContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(),
														  bitmapProperties,
														  bitmap.GetAddressOf()));
		_deviceContext->SetTarget(bitmap.Get());
		_deviceContext->SetDpi(dpiX, dpiY);

		Check(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
								  __uuidof(IDWriteFactory),
								  &_dwriteFactory));
	}
	catch (...) { }
}

void Application::Draw() NOEXCEPT
{
	using namespace D2D1;
	using namespace MTL;
	using namespace std;

	auto title = wstring(L"AutoLogin");
	auto description = wstring(
		L"Приложение работает в автоматическом режиме.\r\n"
		L"Как только будет установлено соединение с WiFi сетью, будет запущен процесс автоматической авторизации.\r\n"
		L"Вам останется только дождаться уведомления об успешном соединении.");

	auto size = _deviceContext->GetPixelSize();

	FLOAT dpiX,
		  dpiY;

	_deviceContext->GetDpi(&dpiX, &dpiY);

	auto scaleFactor = 96.0f / dpiX;
	auto margin = scaleFactor * 12.0f;

	ComPtr<IDWriteTextFormat> titleTextFormat;
	_dwriteFactory->CreateTextFormat(L"Segoe UI",
									 nullptr,
									 DWRITE_FONT_WEIGHT_NORMAL,
									 DWRITE_FONT_STYLE_NORMAL,
									 DWRITE_FONT_STRETCH_NORMAL,
									 14.0f,
									 L"en-US",
									 &titleTextFormat);

	ComPtr<IDWriteTextFormat> descriptionTextFormat;
	_dwriteFactory->CreateTextFormat(L"Segoe UI",
									 nullptr,
									 DWRITE_FONT_WEIGHT_NORMAL,
									 DWRITE_FONT_STYLE_NORMAL,
									 DWRITE_FONT_STRETCH_NORMAL,
									 10.0f,
									 L"ru-RU",
									 &descriptionTextFormat);

	ComPtr<IDWriteTextLayout> titleTextLayout;
	_dwriteFactory->CreateTextLayout(title.data(),
									 title.size(),
									 titleTextFormat.Get(),
									 size.width / 2 - margin,
									 0.0f,
									 &titleTextLayout);

	ComPtr<IDWriteTextLayout> descriptionTextLayout;
	_dwriteFactory->CreateTextLayout(description.data(),
									 description.size(),
									 descriptionTextFormat.Get(),
									 scaleFactor * size.width - margin - margin,
									 size.height,
									 &descriptionTextLayout);

	DWRITE_TEXT_METRICS titleMetrics = {};
	titleTextLayout->GetMetrics(&titleMetrics);

	ComPtr<ID2D1SolidColorBrush> brush;
	_deviceContext->CreateSolidColorBrush(ColorF(ColorF::White),
										  &brush);

	_deviceContext->BeginDraw();

	_deviceContext->DrawTextLayout(Point2F(margin, margin + margin),
								   titleTextLayout.Get(),
								   brush.Get());

	_deviceContext->DrawTextLayout(Point2F(margin, margin + margin + titleMetrics.height),
								   descriptionTextLayout.Get(),
								   brush.Get());
	_deviceContext->EndDraw();

	_swapChain->Present(1, 0);
}

void Application::RegisterBackgroundTask() NOEXCEPT
{
	using namespace ABI::Windows::ApplicationModel::Background;
	using namespace ABI::Windows::Foundation::Collections;
	using namespace ABI::Windows::Foundation;
	using namespace MTL;
	using namespace MTL;

	ComPtr<IBackgroundExecutionManagerStatics> backgroundExecutionManagerStatics;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundExecutionManager).Get(),
						 &backgroundExecutionManagerStatics);

	ComPtr<IAsyncOperation<BackgroundAccessStatus>> backgroundAccessStatusAsyncOperation;
	backgroundExecutionManagerStatics->RequestAccessAsync(&backgroundAccessStatusAsyncOperation);

	GetTask(backgroundAccessStatusAsyncOperation.Get()).then(
		[]
		(BackgroundAccessStatus) ->
		void
		{
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

			backgroundTaskBuilder->put_Name(HString(L"AutoLoginTask").Detach());
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
		});
}

int CALLBACK WinMain(HINSTANCE,
					 HINSTANCE,
					 LPSTR,
					 int) NOEXCEPT
{
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::ApplicationModel::Core;
	using namespace ABI::Windows::ApplicationModel;
	using namespace MTL;
	using namespace MTL;

	try
	{
		Check(RoInitialize(RO_INIT_MULTITHREADED));

		ComPtr<ICoreApplication> coreApplication;
		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
								   &coreApplication));

		Check(coreApplication->Run(new Application()));
	}
	catch (const ComException& comException)
	{
		//TODO WriteLog
	}
	catch (const std::bad_alloc& badAlloc)
	{
		//TODO WriteLog
	}
}
