#pragma once
#include <EasyFree_h.h>
#include <d2d1_1.h>
#include <dxgi1_2.h>
#include <dwrite.h>
#include <windows.applicationmodel.core.h>
#include <macro.h>
#include <AuthStatus.h>
#include <MTL.h>

namespace EasyFree
{
	namespace Implementations
	{
		class Application final : public MTL::RuntimeClass<ABI::EasyFree::IApplication,
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
			Internals::AuthStatus::Enum _authStatus = Internals::AuthStatus::None;

			MTL::ComPtr<ABI::Windows::UI::Core::ICoreWindow> _coreWindow;
			MTL::ComPtr<ID2D1DeviceContext> _deviceContext;
			MTL::ComPtr<IDXGISwapChain1> _swapChain;
			MTL::ComPtr<IDWriteFactory> _dwriteFactory;
			MTL::ComPtr<IDWriteTextFormat> _titleTextFormat;
			MTL::ComPtr<IDWriteTextFormat> _descriptionTextFormat;

			void InitContext() NOEXCEPT;
			void Draw() NOEXCEPT;
			Concurrency::task<void> RegisterBackgroundTask() NOEXCEPT;
			
			MTL::ComPtr<IDWriteTextLayout> GetTitleLayout(FLOAT fontSize, D2D1_SIZE_F size);
			MTL::ComPtr<IDWriteTextLayout> GetDescriptionLayout(FLOAT fontSize, D2D1_SIZE_F size);
		};
	}
}
