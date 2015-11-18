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
					*handler = static_cast<ProgressHandler*>(_progressHandler);
					return S_OK;
				}

				STDMETHODIMP put_Completed(CompleteHandler* handler) NOEXCEPT override
				{
					_completeHandler.Attach(handler);
					return S_OK;
				}

				STDMETHODIMP get_Completed(CompleteHandler** handler) NOEXCEPT override
				{
					*handler = static_cast<CompleteHandler*>(_completeHandler);
					return S_OK;
				}

				STDMETHODIMP GetResults(Results** results) NOEXCEPT override
				{
					*results = static_cast<Results*>(_results);
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
						if (_completeHandler) Check(_completeHandler->Invoke(this, status));
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
				explicit RetryHttpFilter(IHttpFilter* innerFilter) NOEXCEPT
					: _innerFilter(innerFilter) { }

				STDMETHODIMP QueryInterface(const GUID& guid, void** result) override
				{
					if (SUCCEEDED(RuntimeClass<IHttpFilter>::QueryInterface(guid, result)))
					{
						return S_OK;
					}

					return static_cast<IHttpFilter*>(_innerFilter)->QueryInterface(guid, result);
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
						ComPtr<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>> asyncOperation;

						Check(_innerFilter->SendRequestAsync(request,
															 &asyncOperation));

						auto completeCallback = CreateCallback<IAsyncOperationWithProgressCompletedHandler<HttpResponseMessage*, HttpProgress>>(
							[result]
							(IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>* asyncInfo, AsyncStatus status)->
							HRESULT
							{
								IHttpResponseMessage* response;
								asyncInfo->GetResults(&response);
								reinterpret_cast<RequestAsyncOperation*>(result.Get())->SetResults(response, status);
								return S_OK;
							});

						asyncOperation->put_Completed(completeCallback.Get());

						*operation = static_cast<IAsyncOperationWithProgress<HttpResponseMessage*, HttpProgress>*>(result);
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
			};
		}
	}
}
