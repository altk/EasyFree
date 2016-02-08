#pragma once
#include <string>

namespace AutoLogin
{
    namespace CrossPlatform
    {
        class PostData final
        {
            std::wstring _action;
            std::wstring _data;
        public:
            PostData() NOEXCEPT
                : _action(),
                  _data() { }

            PostData(std::wstring action,
                     std::wstring data) NOEXCEPT
                : _action(move(action)),
                  _data(move(data)) {}

            PostData(const PostData &other) NOEXCEPT
                : _action(other._action),
                  _data(other._data) {}

            PostData(PostData &&other)
                : _action(move(other._action)),
                  _data(move(other._data)) {}

            PostData& operator=(const PostData &other)
            {
                if (this != &other)
                {
                    _action = other._action;
                    _data = other._data;
                }
                return *this;
            }

            PostData& operator=(PostData &&other)
            {
                if (this != &other)
                {
                    _action = move(other._action);
                    _data = move(other._data);
                }
                return *this;
            }

            const std::wstring& getAction() const NOEXCEPT
            {
                return _action;
            }

            const std::wstring& getData() const NOEXCEPT
            {
                return _data;
            }

            bool empty() const NOEXCEPT
            {
                return _action.empty() && _data.empty();
            }
        };
    }
}
