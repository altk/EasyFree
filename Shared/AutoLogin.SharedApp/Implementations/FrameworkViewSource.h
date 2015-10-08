#pragma once
#include <Windows.ApplicationModel.core.h>
#include <MTL\Implements\RuntimeClass.h>

namespace AutoLogin
{
	namespace Implementations
	{
		class FrameworkViewSource : public MTL::Implements::RuntimeClass<ABI::Windows::ApplicationModel::Core::IFrameworkViewSource>
		{
		public:
			STDMETHODIMP GetRuntimeClassName(HSTRING* result) override;

			STDMETHODIMP CreateView(ABI::Windows::ApplicationModel::Core::IFrameworkView** viewProvider) override;
		};
	}
}
