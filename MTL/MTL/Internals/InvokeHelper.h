#pragma once
#include <utility>
#include <MTL\Implements\ComClass.h>

namespace MTL
{
	namespace Internals
	{
		template <typename TDelegateInterface, typename TCallback, typename ... TArgs>
		struct InvokeHelper final : Implements::ComClass<TDelegateInterface>
		{
			explicit InvokeHelper(TCallback&& callback) noexcept
				: _callback(std::forward<TCallback>(callback))
			{
			}

			InvokeHelper(const InvokeHelper&) = default;
			InvokeHelper(InvokeHelper&&) = default;
			InvokeHelper& operator=(const InvokeHelper&) = default;
			InvokeHelper& operator=(InvokeHelper&&) = default;

			STDMETHODIMP Invoke(TArgs... args) noexcept override
			{
				return _callback(std::forward<TArgs>(args)...);
			}

			TCallback _callback;
		};

		template <typename TDelegateInterface, typename TCallback, typename ... TArgs>
		inline InvokeHelper<TDelegateInterface, TCallback, TArgs...> ÑreateInvokeHelper(TCallback&& callback, types_pack<TArgs...>) noexcept
		{
			return InvokeHelper<TDelegateInterface, TCallback, TArgs...>(std::forward<TCallback>(callback));
		}
	}
}
