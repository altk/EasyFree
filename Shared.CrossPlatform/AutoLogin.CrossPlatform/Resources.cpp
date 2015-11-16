#include "pch.h"
#include <Resources.h>

using namespace AutoLogin::CrossPlatform;

const wchar_t* const Resources::Title = L"Auto Login";

const wchar_t* const Resources::AuthSuccess =
L"Авторизация выполнена успешно.\r\n"
L"Приятного использования интернета.";

const wchar_t* const Resources::AuthFail =
L"Произошла ошибка при авторизации.\r\n"
L"Попробуйте переподключиться к текущему соединению.";

const wchar_t* const Resources::Description = 
L"Приложение работает в автономном режиме.\r\n"
L"Как только будет установлено соединение с Wi-Fi сетью, запуститься процесс авторизации.\r\n"
L"Приятного использования интернета :-)\r\n\r\n"
L"ВНИМАНИЕ: в режиме энергосбережения выполнение автоматической авторизации невозможно.";

const wchar_t* const Resources::RegistrationNeed =
L"Для использования текущего интернет соединения необходимо пройти регистрацию.";

const wchar_t* const Resources::Unlicensed = 
L"Произошла внутренняя ошибка."