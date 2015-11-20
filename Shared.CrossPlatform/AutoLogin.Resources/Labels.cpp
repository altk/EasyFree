﻿#include "pch.h"
#include "Labels.h"

using namespace AutoLogin::Resources;

const std::wstring Labels::Title = L"Auto Login";

const std::wstring Labels::AuthSuccess =
L"Авторизация выполнена успешно.\r\n"
L"Приятного использования интернета.";

const std::wstring Labels::AuthFail =
L"Произошла ошибка при авторизации.\r\n"
L"Попробуйте переподключиться к текущему соединению.";

const std::wstring Labels::Description = 
L"Приложение работает в автономном режиме.\r\n"
L"Как только будет установлено соединение с Wi-Fi сетью, запуститься процесс авторизации.\r\n"
L"Приятного использования интернета :-)\r\n\r\n"
L"ВНИМАНИЕ: в режиме энергосбережения выполнение автоматической авторизации невозможно.";

const std::wstring Labels::RegistrationNeed =
L"Для использования текущего интернет соединения необходимо пройти регистрацию.";

const std::wstring Labels::Unlicensed =
L"Произошла внутренняя ошибка.";