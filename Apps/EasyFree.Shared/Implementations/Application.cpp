﻿#include "pch.h"
#include "Application.h"
#include <roapi.h>
#include <memory>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite.h>
#include <windows.graphics.display.h>
#include <windows.applicationmodel.background.h>
#include <MTL.h>

using namespace EasyFree::Implementations;

HRESULT Application::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
	using namespace MTL;

	try
	{
		*className = HString(RuntimeClass_EasyFree_Application).Detach();
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
		(ICoreWindow*, IVisibilityChangedEventArgs* args)->
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
				if (_deviceContext)
				{
					Draw();
				}

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
		return comException.GetResult();
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

	try
	{
		RegisterBackgroundTask();

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

	auto size = _deviceContext->GetPixelSize();

	FLOAT dpiX,
		  dpiY;

	_deviceContext->GetDpi(&dpiX, &dpiY);

	auto scaleFactor = dpiX / 96.0f;
	auto margin = 4;
	auto width = size.width / scaleFactor - 2 * margin;

	auto titleTextLayout = GetTitleLayout(14.0f, SizeF(width));
	auto descriptionTextLayout = GetDescriptionLayout(9.0f, SizeF(width));

	ComPtr<ID2D1SolidColorBrush> brush;
	Check(_deviceContext->CreateSolidColorBrush(ColorF(ColorF::White),
												&brush));

	DWRITE_TEXT_METRICS titleMetrics = {};
	Check(titleTextLayout->GetMetrics(&titleMetrics));

	_deviceContext->BeginDraw();

	_deviceContext->DrawTextLayout(Point2F(margin, margin + margin),
								   titleTextLayout.Get(),
								   brush.Get());

	_deviceContext->DrawTextLayout(Point2F(margin, margin + margin + titleMetrics.height),
								   descriptionTextLayout.Get(),
								   brush.Get());

	Check(_deviceContext->EndDraw());

	Check(_swapChain->Present(1, 0));
}

Concurrency::task<void> Application::RegisterBackgroundTask() NOEXCEPT
{
	using namespace ABI::Windows::ApplicationModel::Background;
	using namespace ABI::Windows::Foundation::Collections;
	using namespace ABI::Windows::Foundation;
	using namespace MTL;
	using namespace MTL;

	try
	{
		ComPtr<IBackgroundExecutionManagerStatics> backgroundExecutionManagerStatics;
		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundExecutionManager).Get(),
								   &backgroundExecutionManagerStatics));

		ComPtr<IAsyncOperation<BackgroundAccessStatus>> backgroundAccessStatusAsyncOperation;
		Check(backgroundExecutionManagerStatics->RequestAccessAsync(&backgroundAccessStatusAsyncOperation));

		return GetTask(backgroundAccessStatusAsyncOperation.Get()).then(
			[]
			(BackgroundAccessStatus status) NOEXCEPT ->
			void
			{
				try
				{
					if (status == BackgroundAccessStatus_Denied) return;

					ComPtr<IBackgroundTaskRegistrationStatics> backgroundTaskRegistrationStatics;
					Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundTaskRegistration).Get(),
											   &backgroundTaskRegistrationStatics));

					ComPtr<IMapView<GUID, IBackgroundTaskRegistration*>> taskRegistrations;
					Check(backgroundTaskRegistrationStatics->get_AllTasks(&taskRegistrations));

					ComPtr<IIterable<IKeyValuePair<GUID, IBackgroundTaskRegistration*>*>> registrationsIterable;
					Check(taskRegistrations.As(&registrationsIterable));

					if (begin(registrationsIterable.Get()) != end(registrationsIterable.Get())) return;

					ComPtr<IBackgroundTaskBuilder> backgroundTaskBuilder;
					Check(ActivateInstance<IBackgroundTaskBuilder>(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundTaskBuilder).Get(),
																   &backgroundTaskBuilder));

					Check(backgroundTaskBuilder->put_Name(HStringReference(L"EasyFreeTask").Get()));
					Check(backgroundTaskBuilder->put_TaskEntryPoint(HStringReference(L"EasyFree.Background.LoginTask").Get()));

					ComPtr<ISystemTriggerFactory> systemTriggerActivationFactory;
					Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_SystemTrigger).Get(),
											   &systemTriggerActivationFactory));

					ComPtr<ISystemTrigger> systemTrigger;
					Check(systemTriggerActivationFactory->Create(SystemTriggerType_NetworkStateChange,
																 false,
																 &systemTrigger));

					ComPtr<IBackgroundTrigger> backgroundTrigger;
					Check(systemTrigger.As(&backgroundTrigger));

					Check(backgroundTaskBuilder->SetTrigger(backgroundTrigger.Get()));

					ComPtr<IBackgroundTaskRegistration> taskRegistration;
					Check(backgroundTaskBuilder->Register(&taskRegistration));
				}
				catch (...) {}
			});
	}
	catch (...)
	{
		return Concurrency::task_from_result();
	}
}

MTL::ComPtr<IDWriteTextLayout> Application::GetTitleLayout(FLOAT fontSize, D2D1_SIZE_F size)
{
	using namespace D2D1;
	using namespace MTL;
	using namespace std;

	if (!_titleTextFormat)
	{
		Check(_dwriteFactory->CreateTextFormat(L"Segoe UI",
											   nullptr,
											   DWRITE_FONT_WEIGHT_NORMAL,
											   DWRITE_FONT_STYLE_NORMAL,
											   DWRITE_FONT_STRETCH_NORMAL,
											   fontSize,
											   L"en-US",
											   &_titleTextFormat));
	}

	const wchar_t title[] = L"EasyFree";
	ComPtr<IDWriteTextLayout> titleTextLayout;
	Check(_dwriteFactory->CreateTextLayout(title,
										   extent<decltype(title)>::value,
										   _titleTextFormat.Get(),
										   size.width,
										   size.height,
										   &titleTextLayout));

	return titleTextLayout;
}

MTL::ComPtr<IDWriteTextLayout> Application::GetDescriptionLayout(FLOAT fontSize, D2D1_SIZE_F size)
{
	using namespace D2D1;
	using namespace MTL;
	using namespace std;

	if (!_descriptionTextFormat)
	{
		Check(_dwriteFactory->CreateTextFormat(L"Segoe UI",
											   nullptr,
											   DWRITE_FONT_WEIGHT_NORMAL,
											   DWRITE_FONT_STYLE_NORMAL,
											   DWRITE_FONT_STRETCH_NORMAL,
											   fontSize,
											   L"ru-RU",
											   &_descriptionTextFormat));
	}

	const wchar_t description[] =
			L"Приложение работает в автоматическом режиме.\r\n"
			L"Как только будет установлено соединение с WiFi сетью, будет запущен процесс автоматической авторизации. "
			L"Вам останется только дождаться уведомления об успешном соединении.";

	ComPtr<IDWriteTextLayout> descriptionTextLayout;
	Check(_dwriteFactory->CreateTextLayout(description,
										   extent<decltype(description)>::value,
										   _descriptionTextFormat.Get(),
										   size.width,
										   size.height,
										   &descriptionTextLayout));

	return descriptionTextLayout;
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
	catch (const ComException&)
	{
		//TODO WriteLog
	}
	catch (const std::bad_alloc&)
	{
		//TODO WriteLog
	}
}