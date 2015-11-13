#include "pch.h"
#include <activation.h>
#include <EasyFree.Background_h.h>
#include <macro.h>
#include <MTL.h>
#include <Implementations\LoginTask.h>

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) NOEXCEPT
{
	return TRUE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING activatableClassId,
									   IActivationFactory** factory) NOEXCEPT
{
	using namespace ABI::EasyFree;
	using namespace EasyFree::Background::Implementations;
	using namespace MTL;

	*factory = nullptr;
	
	if (nullptr == activatableClassId || nullptr == factory)
	{
		return E_INVALIDARG;
	}

	if (HString(activatableClassId) != HString(RuntimeClass_EasyFree_Background_LoginTask))
	{
		return E_NOINTERFACE;
	}

	if (nullptr == (*factory = new(std::nothrow) ActivationFactory<LoginTask>()))
	{
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

HRESULT WINAPI DllCanUnloadNow() NOEXCEPT
{
	//TODO реализовать правильное уведомление
	return S_OK;
}
