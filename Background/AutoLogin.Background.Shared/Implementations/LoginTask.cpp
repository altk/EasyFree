#include <pch.h>
#include "LoginTask.h"
#include <MTL\Wrappers\HString.h>
#include <MTL\Wrappers\HStringReference.h>

HRESULT AutoLogin::Background::Implementations::LoginTask::GetRuntimeClassName(HSTRING* className)
{
	using namespace MTL::Wrappers;

	*className = HString(RuntimeClass_AutoLogin_Background_LoginTask).Detach();

	return S_OK;
}

HRESULT AutoLogin::Background::Implementations::LoginTask::Run(ABI::Windows::ApplicationModel::Background::IBackgroundTaskInstance* taskInstance)
{
	//taskInstance->getK

	return S_OK;
}