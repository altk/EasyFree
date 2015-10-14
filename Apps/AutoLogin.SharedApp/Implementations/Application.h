#pragma once
#include <macro.h>
#include <d2d1_1.h>
#include <dxgi1_2.h>
#include <windows.applicationmodel.h>
#include <windows.applicationmodel.core.h>
#include <MTL\Implements\RuntimeClass.h>
#include <MTL\Client\ComPtr.h>
#include <AutoLogin_h.h>

namespace AutoLogin
{
	class Application final : public MTL::Implements::RuntimeClass<ABI::AutoLogin::IApplication,
																   ABI::Windows::ApplicationModel::Core::IFrameworkViewSource,
																   ABI::Windows::ApplicationModel::Core::IFrameworkView>
	{
	public:
		STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override final;
		STDMETHODIMP CreateView(ABI::Windows::ApplicationModel::Core::IFrameworkView** viewProvider) NOEXCEPT override final;
		STDMETHODIMP Initialize(ABI::Windows::ApplicationModel::Core::ICoreApplicationView* applicationView) NOEXCEPT override final;
		STDMETHODIMP SetWindow(ABI::Windows::UI::Core::ICoreWindow* window) NOEXCEPT override final;
		STDMETHODIMP Load(HSTRING entryPoint) NOEXCEPT override final;
		STDMETHODIMP Run() NOEXCEPT override final;
		STDMETHODIMP Uninitialize() NOEXCEPT override final;
	private:
		MTL::Client::ComPtr<ABI::Windows::UI::Core::ICoreWindow> _coreWindow;
		MTL::Client::ComPtr<ID2D1DeviceContext> _deviceContext;
		MTL::Client::ComPtr<IDXGISwapChain1> _swapChain;

		void InitContext() NOEXCEPT;
		void Draw() NOEXCEPT;
		static void Get() NOEXCEPT;
	};
}
