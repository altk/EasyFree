#pragma once
#include <AutoLogin_h.h>
#include <d2d1_1.h>
#include <dxgi1_2.h>
#include <dwrite.h>
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
			MTL::HString _launchArgument;

			MTL::ComPtr<ABI::Windows::UI::Core::ICoreWindow> _coreWindow;
			MTL::ComPtr<ID2D1DeviceContext> _deviceContext;
			MTL::ComPtr<IDXGISwapChain1> _swapChain;
			MTL::ComPtr<IDWriteFactory> _dwriteFactory;
			MTL::ComPtr<IDWriteTextFormat> _titleTextFormat;
			MTL::ComPtr<IDWriteTextFormat> _descriptionTextFormat;

			void InitContext() NOEXCEPT;
			void Draw(const std::wstring&) NOEXCEPT;

			MTL::ComPtr<IDWriteTextLayout> GetTextLayout(const std::wstring&,
														 D2D1_SIZE_F,
														 IDWriteTextFormat*) NOEXCEPT;

			static FLOAT GetScaleFactor(ID2D1DeviceContext*) NOEXCEPT;

			static Concurrency::task<std::wstring> RegisterBackgroundTask() NOEXCEPT;
		};
	}
}
