#include "pch.h"
#include "Labels.h"

using namespace AutoLogin::Resources;

const std::wstring Labels::Title = L"Auto Login";

const std::wstring Labels::AuthSuccess =
L"Авторизация выполнена успешно.\r\n"
L"Приятного использования интернета.";

const std::wstring Labels::AuthFail =
L"Произошла ошибка при авторизации.\r\n"
L"Попробуйте переподключиться к текущему соединению.";

const std::wstring Labels::SuccessDescription =
L"Приложение работает в автономном режиме.\r\n"
L"Как только будет установлено соединение с Wi-Fi сетью, запуститься процесс авторизации.\r\n"
L"Приятного использования интернета :-)\r\n\r\n"
L"ВНИМАНИЕ: чтобы включить автоматическую авторизацию в режиме энергосбережения, необходимо разрешить приложению работать в фоновом режиме при экономии заряда.";

const std::wstring Labels::FailDescription =
L"В работе приложения произошла ошибка.\r\n"
L"Для корректной работы необходимо перезапустить приложение.";

const std::wstring Labels::RegisterBackgroundTaskFail =
L"Произошла ошибка при регистрации фоновой задачи.\r\n"
L"Попрообуйте перезапустить приложение.";

const std::wstring Labels::RegisterBackgroundTaskDenied =
L"Невозможно зарегистрировать фоновую задачу.\r\n"
L"Количество приложений, работающих в фоновом режиме достигло максимального значения.\r\n\r\n"
L"Попробуйте отключить часть программ, работающих в фоновом режиме, и перезапустить приложение.";

const std::wstring Labels::RegistrationNeed =
L"Для использования текущего интернет соединения необходимо пройти регистрацию.";

const std::wstring Labels::Unlicensed =
L"Произошла внутренняя ошибка.";