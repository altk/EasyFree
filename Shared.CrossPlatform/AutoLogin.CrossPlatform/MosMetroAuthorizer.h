#pragma once
#include <IAuthorizer.h>

namespace AutoLogin
{
    namespace CrossPlatform
    {
        class MosMetroAuthorizer final : public IAuthorizer
        {
        public:
            virtual Concurrency::task<AuthResult> AuthAsync() NOEXCEPT override;

            virtual bool CanAuth(const std::wstring &connectionName) const NOEXCEPT override
            {
                return connectionName.compare(L"MosMetro_Free") == 0;
            }

            virtual std::wstring GetRegistrationUrl() const NOEXCEPT override
            {
                return GetRegistrationUrlImpl();
            }

        private:
            static std::wstring GetRegistrationUrlImpl() NOEXCEPT
            {
                return L"https://login.wi-fi.ru/am/UI/Login";
            }
        };
    }
}
