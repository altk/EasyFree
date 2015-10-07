#pragma once
#include <utility>
#include <MTL\Client\ComPtr.h>
#include <MTL\Internals\utility.h>
#include <MTL\Internals\InvokeHelper.h>

namespace MTL
{
	namespace Client
	{
		template<typename TDelegateInterface, typename TCallback>
		inline ComPtr<TDelegateInterface> CreateCallback(TCallback&& callback) noexcept
		{
			using namespace Internals;

			static_assert(std::is_base_of<IUnknown, TDelegateInterface>::value && !std::is_base_of<IInspectable, TDelegateInterface>::value, "Delegates objects must be 'IUnknown' base and not 'IInspectable'");

			using methodType = decltype(&TDelegateInterface::Invoke);
			using types = typename arg_traits<methodType>::types;

			auto helper = ÑreateInvokeHelper<TDelegateInterface>(std::forward<TCallback>(callback), types());

			using helperType = decltype(helper);

			return ComPtr<TDelegateInterface>(new helperType(std::move(helper)));
		};

		struct AsyncOperationHelper final
		{
		public:
			template <typename TArgument>
			static auto GetTask(ABI::Windows::Foundation::IAsyncOperation<TArgument>* asyncOperation) noexcept
			{
				using namespace concurrency;
				using namespace ABI::Windows::Foundation::Internal;
				using namespace ABI::Windows::Foundation;

				using TResult = typename GetAbiType<typename IAsyncOperation<TArgument>::TResult_complex>::type;

				task_completion_event<TResult> taskCompletitionEvent;
				auto callback = CreateCallback<IAsyncOperationCompletedHandler<TArgument>>([taskCompletitionEvent](IAsyncOperation<TArgument>* operation, AsyncStatus status)-> HRESULT
				{
					TResult result;
					operation->GetResults(&result);
					taskCompletitionEvent.set(result);
					return S_OK;
				});
				asyncOperation->put_Completed(callback.Get());
				return task<TResult>(taskCompletitionEvent);
			}

			template <typename TArgument, typename TProgress>
			static auto GetTask(ABI::Windows::Foundation::IAsyncOperationWithProgress<TArgument, TProgress>* asyncOperation) noexcept
			{
				using namespace concurrency;
				using namespace ABI::Windows::Foundation::Internal;
				using namespace ABI::Windows::Foundation;

				using TResult = typename GetAbiType<typename IAsyncOperationWithProgress<TArgument, TProgress>::TResult_complex>::type;

				task_completion_event<TResult> taskCompletitionEvent;
				auto callback = CreateCallback<IAsyncOperationWithProgressCompletedHandler<TArgument, TProgress>>([taskCompletitionEvent](IAsyncOperationWithProgress<TArgument, TProgress>* operation, AsyncStatus status)-> HRESULT
				{
					TResult result;
					operation->GetResults(&result);
					taskCompletitionEvent.set(result);
					return S_OK;
				});
				asyncOperation->put_Completed(callback.Get());
				return task<TResult>(taskCompletitionEvent);
			}
		};
	}
}
