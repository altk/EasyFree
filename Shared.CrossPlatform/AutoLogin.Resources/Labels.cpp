#include "pch.h"
#include "Labels.h"

using namespace AutoLogin::Resources;

const wchar_t* const Labels::Title = L"Auto Login";

const wchar_t* const Labels::AuthSuccess =
L"Авторизация выполнена успешно.\r\n"
L"Приятного использования интернета.";

const wchar_t* const Labels::AuthFail =
L"Произошла ошибка при авторизации.\r\n"
L"Попробуйте переподключиться к текущему соединению.";

const wchar_t* const Labels::Description = 
L"Приложение работает в автономном режиме.\r\n"
L"Как только будет установлено соединение с Wi-Fi сетью, запуститься процесс авторизации.\r\n"
L"Приятного использования интернета :-)\r\n\r\n"
L"ВНИМАНИЕ: в режиме энергосбережения выполнение автоматической авторизации невозможно.";

const wchar_t* const Labels::RegistrationNeed =
L"Для использования текущего интернет соединения необходимо пройти регистрацию.";

const wchar_t* const Labels::Unlicensed =
L"Произошла внутренняя ошибка.";