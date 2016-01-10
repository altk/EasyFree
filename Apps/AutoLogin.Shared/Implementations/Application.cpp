#include "pch.h"
#include "Application.h"
#include <roapi.h>
#include <memory>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite.h>
#include <windows.graphics.display.h>
#include <windows.applicationmodel.background.h>
#include <Labels.h>
#include <MTL.h>
#include <AuthStatus.h>
#include <UriUtilities.h>

using namespace D2D1;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::Graphics::Display;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System;
using namespace ABI::Windows::Foundation;
using namespace Concurrency;
using namespace std;
using namespace MTL;
using namespace AutoLogin::Implementations;
using namespace AutoLogin::Resources;
using namespace AutoLogin::Windows;

HRESULT Application::GetRuntimeClassName(HSTRING* className) NOEXCEPT
{
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

HRESULT Application::CreateView(IFrameworkView** viewProvider) NOEXCEPT
{
	*viewProvider = this;
	return S_OK;
}

HRESULT Application::Initialize(ICoreApplicationView* applicationView) NOEXCEPT
{
	EventRegistrationToken token;
	auto callback = CreateCallback<ITypedEventHandler<CoreApplicationView*, IActivatedEventArgs*>>(
		[this]
		(ICoreApplicationView* coreApplicationView, IActivatedEventArgs* args) ->
		HRESULT
		{
			try
			{
				ComPtr<ICoreWindow> coreWindow;
				Check(coreApplicationView->get_CoreWindow(&coreWindow));
				Check(coreWindow->Activate());

				ActivationKind activationKind;
				Check(args->get_Kind(&activationKind));

				switch (activationKind)
				{
					case ActivationKind_Launch:
						{
							ComPtr<ILaunchActivatedEventArgs> launchActivatedArgs;
							Check(args->QueryInterface<ILaunchActivatedEventArgs>(&launchActivatedArgs));

							Check(launchActivatedArgs->get_Arguments(&_launchArgument));
							break;
						}
					default:
						break;
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
		Check(applicationView->add_Activated(callback.Get(),
											 &token));

		return S_OK;
	}
	catch (const ComException& comException)
	{
		return comException.GetResult();
	}
}

HRESULT Application::SetWindow(ICoreWindow* window) NOEXCEPT
{
	_coreWindow.Attach(window);

	auto visibilityChangedCallback = CreateCallback<ITypedEventHandler<CoreWindow*, VisibilityChangedEventArgs*>>(
		[this]
		(ICoreWindow* pCoreWindow, IVisibilityChangedEventArgs* args)->
		HRESULT
		{
			try
			{
				boolean isVisible;
				Check(args->get_Visible(&isVisible));

				if (isVisible)
				{
					if (!static_cast<bool>(_deviceContext))
					{
						InitContext();
					}

					if (_launchArgument)
					{
						auto unescapedArgument = UriUtilities().Unescape(_launchArgument.Get());

						ComPtr<IUriRuntimeClassFactory> uriFactory;
						Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
												   &uriFactory));

						ComPtr<IUriRuntimeClass> launchUri;
						Check(uriFactory->CreateUri(unescapedArgument.Get(),
													&launchUri));

						HString scheme;
						Check(launchUri->get_SchemeName(&scheme));

						if (scheme)
						{
							auto schemeRaw = scheme.GetRawBuffer();
							if (AuthStatus::launchAttributeScheme.compare(schemeRaw) == 0)
							{
								if (AuthStatus::launchAttributeSuccess.compare(unescapedArgument.GetRawBuffer()) == 0)
								{
									Draw(Labels::AuthSuccess);
								}
								else if (AuthStatus::launchAttributeUnlicensed.compare(unescapedArgument.GetRawBuffer()) == 0)
								{
									Draw(Labels::Unlicensed);
								}
								else if(AuthStatus::launchAttributeFail.compare(unescapedArgument.GetRawBuffer()) == 0)
								{
									Draw(Labels::AuthFail);
								}
							}
							else
							{
								if (wcsncmp(schemeRaw, L"http", 4) == 0)
								{
									ComPtr<ILauncherStatics> launcherStatics;
									Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_System_Launcher).Get(),
															   &launcherStatics));

									ComPtr<IAsyncOperation<bool>> launchAsyncOperation;
									Check(launcherStatics->LaunchUriAsync(launchUri.Get(),
																		  &launchAsyncOperation));

									Check(pCoreWindow->Close());
								}

								Draw(Labels::SuccessDescription);
							}
						}
					}
					else
					{
						RegisterBackgroundTask().then([this](task<wstring> task) NOEXCEPT
							{
								try
								{
									Draw(task.get());
								}
								catch (...)
								{
									Draw(Labels::RegisterBackgroundTaskFail);
								}
							});
					}
				}
				else
				{
					lock_guard<mutex> lock(_deviceContextMutex);

					_deviceContext.Release();
					_swapChain.Release();
					_dwriteFactory.Release();
					_titleTextFormat.Release();
					_descriptionTextFormat.Release();
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
	_deviceContext.Release();
	_swapChain.Release();
	_dwriteFactory.Release();
	_titleTextFormat.Release();
	_descriptionTextFormat.Release();
	_coreWindow.Release();
	return S_OK;
}

void Application::InitContext() NOEXCEPT
{
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

		auto scaleFactor = GetScaleFactor(_deviceContext.Get());

		Check(_dwriteFactory->CreateTextFormat(L"Segoe UI",
											   nullptr,
											   DWRITE_FONT_WEIGHT_NORMAL,
											   DWRITE_FONT_STYLE_NORMAL,
											   DWRITE_FONT_STRETCH_NORMAL,
											   14.0f * scaleFactor,
											   L"en-US",
											   &_titleTextFormat));

		Check(_dwriteFactory->CreateTextFormat(L"Segoe UI",
											   nullptr,
											   DWRITE_FONT_WEIGHT_NORMAL,
											   DWRITE_FONT_STYLE_NORMAL,
											   DWRITE_FONT_STRETCH_NORMAL,
											   9.0f * scaleFactor,
											   L"ru-RU",
											   &_descriptionTextFormat));
	}
	catch (...) { }
}

void Application::Draw(const wstring& description) NOEXCEPT
{
	using namespace D2D1;

	try
	{
		lock_guard<mutex> lock(_deviceContextMutex);

		auto scaleFactor = GetScaleFactor(_deviceContext.Get());

		auto size = _deviceContext->GetSize();

		auto margin = 8.0f * scaleFactor;
		auto width = min(size.width, size.height) - 2 * margin;

		auto titleTextLayout = GetTextLayout(Labels::Title,
											 SizeF(width),
											 _titleTextFormat.Get());

		auto descriptionTextLayout = GetTextLayout(description,
												   SizeF(width),
												   _descriptionTextFormat.Get());

		ComPtr<ID2D1SolidColorBrush> brush;
		Check(_deviceContext->CreateSolidColorBrush(ColorF(ColorF::White),
													&brush));

		DWRITE_TEXT_METRICS titleMetrics = {};
		Check(titleTextLayout->GetMetrics(&titleMetrics));

		_deviceContext->BeginDraw();

		_deviceContext->Clear();

		_deviceContext->DrawTextLayout(Point2F(margin, margin),
									   titleTextLayout.Get(),
									   brush.Get());

		_deviceContext->DrawTextLayout(Point2F(margin, 2.0f * margin + titleMetrics.height),
									   descriptionTextLayout.Get(),
									   brush.Get());

		Check(_deviceContext->EndDraw());

		Check(_swapChain->Present(1, 0));
	}
	catch (...) { }
}

ComPtr<IDWriteTextLayout> Application::GetTextLayout(const wstring& text,
													 D2D1_SIZE_F size,
													 IDWriteTextFormat* pTextFormat) NOEXCEPT
{
	ComPtr<IDWriteTextLayout> result;
	try
	{
		Check(_dwriteFactory->CreateTextLayout(text.data(),
											   text.size(),
											   pTextFormat,
											   size.width,
											   size.height,
											   &result));
	}
	catch (...) { }
	return result;
}

FLOAT Application::GetScaleFactor(ID2D1DeviceContext* deviceContext) NOEXCEPT
{
	FLOAT dpiX, dpiY;

	auto pixelSize = deviceContext->GetPixelSize();

	deviceContext->GetDpi(&dpiX, &dpiY);

	auto diagonal = roundf(sqrtf(powf(pixelSize.width / dpiX, 2.0f) + powf(pixelSize.height / dpiY, 2.0f)));

	auto multiplier = 1.0f;

	if (diagonal < 10.0f)
	{
		multiplier = 1.0f;
	}
	else if (diagonal < 10.0f)
	{
		multiplier = 1.25f;
	}
	else if (diagonal < 15.0f)
	{
		multiplier = 1.5f;
	}
	else if (diagonal < 19.0f)
	{
		multiplier = 1.75f;
	}
	else if (diagonal < 23.0f)
	{
		multiplier = 2.0f;
	}
	else if (diagonal < 27.0f)
	{
		multiplier = 2.25f;
	}
	else if (diagonal < 32.0f)
	{
		multiplier = 2.5f;
	}
	else if (diagonal < 37.0f)
	{
		multiplier = 2.75f;
	}
	else if (diagonal < 42.0f)
	{
		multiplier = 3.0f;
	}

	return multiplier;
}

task<wstring> Application::RegisterBackgroundTask() NOEXCEPT
{
	try
	{
		ComPtr<IBackgroundExecutionManagerStatics> backgroundExecutionManagerStatics;
		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundExecutionManager).Get(),
								   &backgroundExecutionManagerStatics));

		ComPtr<IAsyncOperation<BackgroundAccessStatus>> backgroundAccessStatusAsyncOperation;
		Check(backgroundExecutionManagerStatics->RequestAccessAsync(&backgroundAccessStatusAsyncOperation));

		return GetTask(backgroundAccessStatusAsyncOperation.Get()).then([] (BackgroundAccessStatus status) NOEXCEPT -> wstring
			{
				try
				{
					if (status == BackgroundAccessStatus_Denied)
					{
						return Labels::RegisterBackgroundTaskDenied;
					}

					ComPtr<IBackgroundTaskRegistrationStatics> backgroundTaskRegistrationStatics;
					Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundTaskRegistration).Get(),
											   &backgroundTaskRegistrationStatics));

					ComPtr<IMapView<GUID, IBackgroundTaskRegistration*>> taskRegistrations;
					Check(backgroundTaskRegistrationStatics->get_AllTasks(&taskRegistrations));

					if (begin(taskRegistrations.Get()) == end(taskRegistrations.Get()))
					{
						ComPtr<IBackgroundTaskBuilder> backgroundTaskBuilder;
						Check(ActivateInstance<IBackgroundTaskBuilder>(HStringReference(RuntimeClass_Windows_ApplicationModel_Background_BackgroundTaskBuilder).Get(),
																	   &backgroundTaskBuilder));

						Check(backgroundTaskBuilder->put_Name(HStringReference(L"AutoLoginTask").Get()));
						Check(backgroundTaskBuilder->put_TaskEntryPoint(HStringReference(L"AutoLogin.Background.LoginTask").Get()));

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

					return Labels::SuccessDescription;
				}
				catch (...)
				{
					return Labels::RegisterBackgroundTaskFail;
				}
			});
	}
	catch (...)
	{
		return task_from_result(Labels::RegisterBackgroundTaskFail);
	}
}

int CALLBACK WinMain(HINSTANCE,
					 HINSTANCE,
					 LPSTR,
					 int) NOEXCEPT
{
	try
	{
		Check(RoInitialize(RO_INIT_MULTITHREADED));

		ComPtr<ICoreApplication> coreApplication;
		Check(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
								   &coreApplication));

		Check(coreApplication->Run(new Application()));
	}
	catch (const exception&) { }
}
