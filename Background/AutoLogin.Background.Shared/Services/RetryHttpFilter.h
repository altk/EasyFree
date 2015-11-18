#pragma once
#include <windows.web.http.h>
#include <windows.web.http.filters.h>
#include <MTL.h>
#include <macro.h>

namespace AutoLogin
{
	namespace Background
	{
		namespace Services
		{
			class RequestAsyncOperation final : public MTL::RuntimeClass<ABI::Windows::Foundation::IAsyncOperationWithProgress<ABI::Windows::Web::Http::HttpResponseMessage*, ABI::Windows::Web::Http::HttpProgress>>
			{
				using ProgressHandler = ABI::Windows::Foundation::IAsyncOperationProgressHandler<ABI::Windows::Web::Http::HttpResponseMessage*, ABI::Windows::Web::Http::HttpProgress>;
				using CompleteHandler = ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler<ABI::Windows::Web::Http::HttpResponseMessage*, ABI::Windows::Web::Http::HttpProgress>;
				using Results = std::remove_pointer<ABI::Windows::Foundation::Internal::GetAbiType<TResult_complex>::type>::type;

			public:
				STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override
				{
					using namespace MTL;
					try
					{
						*className = HString(L"AutoLogin.Background.Services.RequestAsyncOperation").Detach();
						return S_OK;
					}
					catch (const ComException& comException)
					{
						*className = nullptr;
						return comException.GetResult();
					}
				}

				STDMETHODIMP put_Progress(ProgressHandler* handler) NOEXCEPT override
				{
					_progressHandler.Attach(handler);
					return S_OK;
				}

				STDMETHODIMP get_Progress(ProgressHandler** handler) NOEXCEPT override
				{
					*handler = _progressHandler.Get();
					if (*handler) (*handler)->AddRef();
					return S_OK;
				}

				STDMETHODIMP put_Completed(CompleteHandler* handler) NOEXCEPT override
				{
					_completeHandler.Attach(handler);
					return S_OK;
				}

				STDMETHODIMP get_Completed(CompleteHandler** handler) NOEXCEPT override
				{
					*handler = _completeHandler.Get();
					if (*handler)(*handler)->AddRef();
					return S_OK;
				}

				STDMETHODIMP GetResults(Results** results) NOEXCEPT override
				{
					*results = _results.Get();
					if (*results)(*results)->AddRef();
					return S_OK;
				}

				void SetResults(Results* results, AsyncStatus status) NOEXCEPT
				{
					using namespace MTL;
					using namespace ABI::Windows::Foundation;
					using namespace ABI::Windows::Web::Http;

					try
					{
						_results.Release();
						_results.Attach(results);
						if (_completeHandler)
						{
							Check(_completeHandler->Invoke(this, status));
						}
					}
					catch (...) {}
				}

			private:
				MTL::ComPtr<ProgressHandler> _progressHandler;
				MTL::ComPtr<CompleteHandler> _completeHandler;
				MTL::ComPtr<Results> _results;
			};

			class RetryHttpFilter final : public MTL::RuntimeClass<ABI::Windows::Web::Http::Filters::IHttpFilter>
			{
			public:
				explicit RetryHttpFilter(IHttpFilter* innerFilter,
										 uint_fast16_t retryCount = 3) NOEXCEPT
					: _innerFilter(innerFilter)
					  ,_retryCount(retryCount) { }

				STDMETHODIMP QueryInterface(const GUID& guid, void** result) NOEXCEPT override
				{
					if (SUCCEEDED(RuntimeClass<IHttpFilter>::QueryInterface(guid, result)))
					{
						return S_OK;
					}

					return static_cast<IHttpFilter*>(_innerFilter.Get())->QueryInterface(guid, result);
				}

				STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override
				{
					using namespace MTL;
					try
					{
						*className = HString(L"AutoLogin.Background.Services.RetryHttpFilter").Detach();
						return S_OK;
					}
					catch (const ComException& comException)
					{
						*className = nullptr;
						return comException.GetResult();
					}
				}

				static void SendRequestAsync(MTL::ComPtr<IHttpFilter> innerFilter,
											 MTL::ComPtr<ABI::Windows::Web::Http::IHttpRequestMessage> request,
											 MTL::ComPtr<ABI::Windows::Foundation::IAsyncOperationWithProgress<ABI::Windows::Web::Http::HttpResponseMessage*, ABI::Windows::Web::Http::HttpProgress>> operation,
											 uint_fast16_t maxAttempts,
											 uint_fast16_t attempt = 0)
				{
					using namespace MTL;
					using namespace ABI::Windows::Web::Http;
					using namespace ABI::Windows::Foundation;

					ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> asyncOperation;
					Check(innerFilter->SendRequestAsync(request.Get(),
														&asyncOperation));

					auto completeCallback = CreateCallback<IAsyncOperationWithProgressCompletedHandler<HttpResponseMessage*, HttpProgress>>(
						[innerFilter, request, operation, maxAttempts, attempt]
						(IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>* asyncInfo, AsyncStatus status)->
						HRESULT
						{
							auto requestAsyncOperation = reinterpret_cast<RequestAsyncOperation*>(operation.Get());
							IHttpResponseMessage* response = nullptr;

							try
							{
								switch (status)
								{
									case AsyncStatus::Completed:

										Check(asyncInfo->GetResults(&response));

										boolean isSuccess;
										response->get_IsSuccessStatusCode(&isSuccess);

										if (!isSuccess)
										{
											if (attempt < maxAttempts)
											{
												SendRequestAsync(innerFilter, 
																 request, 
																 operation, 
																 maxAttempts, 
																 attempt + 1);
												return S_OK;
											}
											else
											{
												status = AsyncStatus::Error;
											}
										}

										break;
									case AsyncStatus::Error:
										if (attempt < maxAttempts)
										{
											SendRequestAsync(innerFilter,
															 request, 
															 operation, 
															 maxAttempts, 
															 attempt + 1);
											return S_OK;
										}
										break;
								}
							}
							catch (...)
							{
								status = AsyncStatus::Error;
							}

							requestAsyncOperation->SetResults(response, status);

							return S_OK;
						});

					Check(asyncOperation->put_Completed(completeCallback.Get()));
				}

				STDMETHODIMP SendRequestAsync(ABI::Windows::Web::Http::IHttpRequestMessage* request,
											  ABI::Windows::Foundation::IAsyncOperationWithProgress<ABI::Windows::Web::Http::HttpResponseMessage*, ABI::Windows::Web::Http::HttpProgress>** operation) NOEXCEPT override
				{
					using namespace std;
					using namespace MTL;
					using namespace ABI::Windows::Web::Http;
					using namespace ABI::Windows::Foundation;

					try
					{
						ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> result(new RequestAsyncOperation());

						SendRequestAsync(_innerFilter,
										 CreateComPtr(request),
										 result,
										 _retryCount);

						*operation = result.Detach();
					}
					catch (const ComException& comException)
					{
						*operation = nullptr;
						return comException.GetResult();
					}
					catch (const bad_alloc&)
					{
						*operation = nullptr;
						return E_OUTOFMEMORY;
					}

					return S_OK;
				}

			private:
				MTL::ComPtr<IHttpFilter> _innerFilter;
				uint_fast16_t _retryCount;
			};
		}
	}
}
