#include "pch.h"
#include "MosMetroAuthorizer.h"
#include <MosMetroResponseParcer.h>
#include <robuffer.h>
#include <windows.web.http.h>
#include <windows.storage.streams.h>
#include <windows.networking.connectivity.h>
#include <MTL.h>

using namespace std;
using namespace Concurrency;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::ApplicationModel::Background;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Web::Http::Headers;
using namespace ABI::Windows::Web::Http::Filters;
using namespace ABI::Windows::Web::Http;
using namespace ABI::Windows::Storage::Streams;
using namespace Windows::Storage::Streams;
using namespace MTL;
using namespace AutoLogin::CrossPlatform;

template <>
task<bool> MosMetroAuthorizer<ComPtr<IHttpResponseMessage>>::CheckAsync(ComPtr<IHttpResponseMessage> response,
																		uint_fast16_t statusCode) NOEXCEPT
{
	HttpStatusCode responseStatusCode;
	Check(response->get_StatusCode(&responseStatusCode));

	return task_from_result(statusCode == responseStatusCode);
}

template <>
task<wstring> MosMetroAuthorizer<ComPtr<IHttpResponseMessage>>::GetAuthUrlAsync(ComPtr<IHttpResponseMessage> response) NOEXCEPT
{
	ComPtr<IHttpContent> httpContent;
	ComPtr<IAsyncOperationWithProgress<IBuffer*, UINT64>> readAsBufferOperation;

	Check(response->get_Content(&httpContent));

	Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

	return GetTask(readAsBufferOperation.Get()).then(
		[]
		(ComPtr<IBuffer> buffer) ->
		wstring
		{
			if (!buffer) return wstring();

			ComPtr<IBufferByteAccess> bufferByteAccess;
			Check(buffer.As(&bufferByteAccess));

			byte* content;
			Check(bufferByteAccess->Buffer(&content));

			return MosMetroResponseParser::GetAuthUrl(reinterpret_cast<const char*>(content));
		});
}

template <>
task<wstring> MosMetroAuthorizer<ComPtr<IHttpResponseMessage>>::GetPostContentAsync(ComPtr<IHttpResponseMessage> response) NOEXCEPT
{
	ComPtr<IHttpContent> httpContent;
	ComPtr<IAsyncOperationWithProgress<IBuffer*, UINT64>> readAsBufferOperation;

	Check(response->get_Content(&httpContent));

	Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

	return GetTask(readAsBufferOperation.Get()).then(
		[]
		(ComPtr<IBuffer> buffer) ->
		wstring
		{
			if (!buffer) return wstring();

			ComPtr<IBufferByteAccess> bufferByteAccess;
			Check(buffer->QueryInterface<IBufferByteAccess>(&bufferByteAccess));

			byte* content;
			Check(bufferByteAccess->Buffer(&content));

			return MosMetroResponseParser::GetPostString(reinterpret_cast<char*>(content));
		});
}
