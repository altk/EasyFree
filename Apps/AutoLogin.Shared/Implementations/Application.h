﻿#pragma once
#include <AutoLogin_h.h>
#include <d2d1_1.h>
#include <dxgi1_2.h>
#include <windows.applicationmodel.core.h>
#include <macro.h>
#include <MTL.h>

namespace AutoLogin
{
	namespace Implementations
	{
		class Application final : public MTL::RuntimeClass<ABI::AutoLogin::IApplication,
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
			MTL::ComPtr<ABI::Windows::UI::Core::ICoreWindow> _coreWindow;
			MTL::ComPtr<ID2D1DeviceContext> _deviceContext;
			MTL::ComPtr<IDXGISwapChain1> _swapChain;
			MTL::ComPtr<IDWriteFactory> _dwriteFactory;

			void InitContext() NOEXCEPT;
			void Draw() NOEXCEPT;
			void RegisterBackgroundTask() NOEXCEPT;
		};
	}
}
