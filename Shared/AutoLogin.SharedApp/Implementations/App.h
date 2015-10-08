#pragma once
#include <AutoLogin_h.h>
#include <MTL\Implements\RuntimeClass.h>

class App final :
		public MTL::Implements::RuntimeClass<ABI::AutoLogin::IApp,
											 ABI::Windows::UI::Xaml::IApplicationOverrides>
{
	//STDMETHODIMP 
};
