#include "pch.h"
#include <activation.h>
#include <AutoLogin.Background_h.h>
#include <macro.h>
#include <MTL\Wrappers\HString.h>
#include <Implementations\LoginTask.h>

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) NOEXCEPT
{
	return TRUE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING activatableClassId,
									   IActivationFactory** factory) NOEXCEPT
{
	using namespace ABI::AutoLogin;
	using namespace AutoLogin::Background::Implementations;
	using namespace MTL::Wrappers;

	if (nullptr == activatableClassId || nullptr == factory)
	{
		return E_INVALIDARG;
	}

	if (HString(activatableClassId) != HString(RuntimeClass_AutoLogin_Background_LoginTask))
	{
		return E_NOINTERFACE;
	}

	if (nullptr == (*factory = new(std::nothrow) LoginTaskFactory()))
	{
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

HRESULT WINAPI DllCanUnloadNow() NOEXCEPT
{
	return S_OK;
}
