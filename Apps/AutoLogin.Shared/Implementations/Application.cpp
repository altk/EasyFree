#include "pch.h"
#include "Application.h"
#include <memory>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <windows.graphics.display.h>
#include <MTL\Wrappers\HString.h>
#include <MTL\Wrappers\HStringReference.h>
#include <MTL\Client\Async.h>

#include <gumbo\gumbo.h>
#include <robuffer.h>
#include <windows.web.http.h>
#include <windows.storage.streams.h>

using namespace AutoLogin::Implementations;

const GumboNode* findNodeByTag(const GumboNode* node, GumboTag tag) NOEXCEPT
{
	if (node->v.element.tag == tag)
	{
		return node;
	}

	auto children = &node->v.element.children;
	for (auto i = 0; i < children->length; ++i)
	{
		auto child = static_cast<GumboNode*>(children->data[i]);
		if (GUMBO_NODE_ELEMENT == child->type)
		{
			auto result = findNodeByTag(child, tag);
			if (nullptr != result)
			{
				return result;
			}
		}
	}

	return nullptr;
}

std::string getPostContent(const char* source)
{
	using namespace std;

	auto output = gumbo_parse(source);

	auto formNode = findNodeByTag(output->root, GUMBO_TAG_FORM);

	vector<tuple<const char*, const char*>> params;

	auto children = &formNode->v.element.children;
	for (auto i = 0; i < children->length; ++i)
	{
		auto child = static_cast<GumboNode*>(children->data[i]);
		if (GUMBO_TAG_INPUT == child->v.element.tag)
		{
			tuple<const char*, const char*> idValueTuple;
			auto attributes = &child->v.element.attributes;
			for (auto j = 0; j < attributes->length; ++j)
			{
				auto attribute = static_cast<GumboAttribute*>(attributes->data[j]);
				if (strcmp(attribute->name, "name") == 0)
				{
					get<0>(idValueTuple) = attribute->value;
				}
				if (strcmp(attribute->name, "value") == 0)
				{
					get<1>(idValueTuple) = attribute->value;
				}
				if (nullptr != get<0>(idValueTuple) && nullptr != get<1>(idValueTuple))
				{
					break;
				}
			}
			params.push_back(idValueTuple);
		}
	}

	string result;

	if (!params.empty())
	{
		for (auto& t : params)
		{
			result = result.append(get<0>(t))
					 .append("=")
					 .append(get<1>(t))
					 .append("&");
		}

		result = result.substr(0, result.size() - 1);
	}

	gumbo_destroy_output(&kGumboDefaultOptions, output);

	return result;
}

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

																													  Get();

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

void Application::Get() NOEXCEPT
{
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::Web::Http::Filters;
	using namespace ABI::Windows::Web::Http;
	using namespace ABI::Windows::Storage::Streams;
	using namespace Windows::Storage::Streams;
	using namespace MTL::Client;
	using namespace MTL::Wrappers;

	ComPtr<IHttpClientFactory> httpClientFactory;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_Web_Http_HttpClient).Get(),
						 &httpClientFactory);

	ComPtr<IHttpFilter> httpFilter;
	ActivateInstance<IHttpFilter>(HStringReference(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter).Get(),
								  &httpFilter);

	ComPtr<IHttpClient> httpClient;
	httpClientFactory->Create(httpFilter.Get(),
							  &httpClient);

	ComPtr<IUriRuntimeClassFactory> uriFactory;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
						 &uriFactory);

	ComPtr<IUriRuntimeClass> uri;
	uriFactory->CreateUri(HStringReference(L"https://login.wi-fi.ru/am/UI/Login?org=mac&service=coa&client_mac=c8-d1-0b-01-24-e1&ForceAuth=true").Get(),
						  &uri);

	ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> getAsyncOperation;
	httpClient->GetAsync(uri.Get(),
						 &getAsyncOperation);

	auto getStringTask = GetTask(getAsyncOperation.Get());

	getStringTask.then([](IHttpResponseMessage* result)-> void
					   {
						   ComPtr<IHttpResponseMessage> response(result);

						   ComPtr<IHttpContent> httpContent;
						   response->get_Content(&httpContent);

						   ComPtr<IAsyncOperationWithProgress<IBuffer*, ULONGLONG>> readAsBufferOperation;
						   httpContent->ReadAsBufferAsync(&readAsBufferOperation);

						   ComPtr<IBuffer> buffer(GetTask(readAsBufferOperation.Get()).get());

						   ComPtr<IBufferByteAccess> bufferByteAccess;
						   buffer.As(&bufferByteAccess);

						   byte* content;
						   bufferByteAccess->Buffer(&content);

						   UINT32 lenght;
						   buffer->get_Length(&lenght);

						   auto t = getPostContent(reinterpret_cast<const char*>(content));
						   OutputDebugStringA(t.data());
					   });
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
