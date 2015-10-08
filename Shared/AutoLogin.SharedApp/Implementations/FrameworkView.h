#pragma once
#include <Windows.ApplicationModel.core.h>
#include <MTL\implements\RuntimeClass.h>

namespace AutoLogin
{
	namespace Implementations
	{
		class FrameworkView : public MTL::Implements::RuntimeClass<ABI::Windows::ApplicationModel::Core::IFrameworkView>
		{
			ABI::Windows::UI::Core::ICoreWindow* _pCoreWindow;
		public:
			STDMETHODIMP GetRuntimeClassName(HSTRING* result) override;

			STDMETHODIMP Initialize(ABI::Windows::ApplicationModel::Core::ICoreApplicationView* applicationView) override;

			STDMETHODIMP SetWindow(ABI::Windows::UI::Core::ICoreWindow* window) override;

			STDMETHODIMP Load(HSTRING entryPoint) override;

			STDMETHODIMP Run() override;

			STDMETHODIMP Uninitialize() override;
		};
	}
}
