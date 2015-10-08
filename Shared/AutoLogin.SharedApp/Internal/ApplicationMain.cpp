#include "pch.h"
#include <roapi.h>
#include <Windows.ApplicationModel.h>
#include <MTL\Client\ComPtr.h>
#include <MTL\Wrappers\HStringReference.h>
#include <Implementations\FrameworkViewSource.h>

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) noexcept
{
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::ApplicationModel::Core;
	using namespace ABI::Windows::ApplicationModel;
	using namespace MTL::Client;
	using namespace MTL::Wrappers;
	using namespace AutoLogin::Implementations;

	RoInitialize(RO_INIT_MULTITHREADED);

	ComPtr<ICoreApplication> coreApplication;
	GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
						 &coreApplication);

	ComPtr<FrameworkViewSource> viewSource(new FrameworkViewSource());
	coreApplication->Run(viewSource.Get());
}
