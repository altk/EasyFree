#include "pch.h"
#include "AuthStatus.h"

using namespace AutoLogin::CrossPlatform;

const wchar_t* const AuthStatus::launchAttributeSuccess = L"autologin://success";

const wchar_t* const AuthStatus::launchAttributeFail = L"autologin://fail";

const wchar_t* const AuthStatus::launchAttributeUnlicensed = L"autologin://unlicensed";

const wchar_t* const AuthStatus::launchAttributeScheme = L"autologin";