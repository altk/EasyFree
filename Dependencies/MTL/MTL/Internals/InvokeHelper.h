#pragma once
#include <utility>
#include <macro.h>
#include <MTL\Implements\ComClass.h>

namespace MTL
{
	namespace Internals
	{
		template <typename TDelegateInterface, typename TCallback, typename ... TArgs>
		class InvokeHelper final : public Implements::ComClass<TDelegateInterface>
		{
		public:
			explicit InvokeHelper(TCallback&& callback) NOEXCEPT
				: _callback(std::forward<TCallback>(callback)) { }

			InvokeHelper(const InvokeHelper& other) NOEXCEPT
				: _callback(other._callback) { }

			InvokeHelper(InvokeHelper&& other) NOEXCEPT
				: _callback(std::move(other._callback)) { }

			InvokeHelper& operator=(const InvokeHelper& other) NOEXCEPT
			{
				if (this != &other)
				{
					_callback = other._callback;
				}
				return *this;
			}

			InvokeHelper& operator=(InvokeHelper&& other) NOEXCEPT
			{
				if (this != &other)
				{
					_callback = std::move(other._callback);
				}
				return *this;
			}

			STDMETHODIMP Invoke(TArgs ... args) NOEXCEPT override
			{
				return _callback(std::forward<TArgs>(args)...);
			}

		private:
			TCallback _callback;
		};

		template <typename TDelegateInterface, typename TCallback, typename ... TArgs>
		inline InvokeHelper<TDelegateInterface, TCallback, TArgs...> ÑreateInvokeHelper(TCallback&& callback, types_pack<TArgs...>) NOEXCEPT
		{
			return InvokeHelper<TDelegateInterface, TCallback, TArgs...>(std::forward<TCallback>(callback));
		}
	}
}
