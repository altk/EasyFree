#include "pch.h"
#include "MosMetroAuthorizer.h"
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
task<bool> MosMetroAuthorizer<IHttpResponseMessage*>::CheckAsync(IHttpResponseMessage* response,
																 uint_fast16_t statusCode) NOEXCEPT
{
	HttpStatusCode responseStatusCode;
	Check(response->get_StatusCode(&responseStatusCode));

	return task_from_result(statusCode == responseStatusCode);
}

template <>
task<string> MosMetroAuthorizer<IHttpResponseMessage*>::GetContentAsync(IHttpResponseMessage* response) NOEXCEPT
{
	ComPtr<IHttpContent> httpContent;
	ComPtr<IAsyncOperationWithProgress<IBuffer*, ULONGLONG>> readAsBufferOperation;

	Check(response->get_Content(&httpContent));

	Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

	return GetTask(readAsBufferOperation.Get()).then(
		[]
		(IBuffer* buffer) ->
		string
		{
			if (nullptr == buffer)
			{
				return string();
			}

			ComPtr<IBufferByteAccess> bufferByteAccess;
			Check(buffer->QueryInterface<IBufferByteAccess>(&bufferByteAccess));

			byte* content;
			Check(bufferByteAccess->Buffer(&content));

			UINT32 lenght;
			Check(buffer->get_Length(&lenght));

			return string(content, content + lenght);
		});
}
