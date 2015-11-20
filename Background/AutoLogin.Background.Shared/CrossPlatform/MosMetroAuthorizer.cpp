#include "pch.h"
#include "MosMetroAuthorizer.h"
#include <MosMetroResponseParcer.h>
#include <algorithm>
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
task<wstring> MosMetroAuthorizer<IHttpResponseMessage*>::GetAuthUrlAsync(IHttpResponseMessage* response) NOEXCEPT
{
	ComPtr<IHttpContent> httpContent;
	ComPtr<IAsyncOperationWithProgress<IBuffer*, UINT64>> readAsBufferOperation;

	Check(response->get_Content(&httpContent));

	Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

	return GetTask(readAsBufferOperation.Get()).then(
		[]
		(IBuffer* buffer) ->
		wstring
		{
			if (nullptr == buffer) return wstring();

			ComPtr<IBufferByteAccess> bufferByteAccess;
			Check(buffer->QueryInterface<IBufferByteAccess>(&bufferByteAccess));

			byte* content;
			Check(bufferByteAccess->Buffer(&content));

			return wstring(L"https://login.wi-fi.ru/am/UI/Login?org=mac&service=coa&client_mac=c8-d1-0b-01-24-e1&ForceAuth=true");

			//return MosMetroResponseParser::GetAuthUrl(reinterpret_cast<const char*>(content));
		});
}

template <>
task<wstring> MosMetroAuthorizer<IHttpResponseMessage*>::GetPostContentAsync(IHttpResponseMessage* response) NOEXCEPT
{
	HttpStatusCode responseStatusCode;
	Check(response->get_StatusCode(&responseStatusCode));

	ComPtr<IHttpContent> httpContent;
	ComPtr<IAsyncOperationWithProgress<IBuffer*, UINT64>> readAsBufferOperation;

	Check(response->get_Content(&httpContent));

	Check(httpContent->ReadAsBufferAsync(&readAsBufferOperation));

	return GetTask(readAsBufferOperation.Get()).then(
		[]
		(IBuffer* buffer) ->
		wstring
		{
			if (nullptr == buffer) return wstring();

			ComPtr<IBufferByteAccess> bufferByteAccess;
			Check(buffer->QueryInterface<IBufferByteAccess>(&bufferByteAccess));

			byte* content;
			Check(bufferByteAccess->Buffer(&content));
			
			UINT32 lenght;
			Check(buffer->get_Length(&lenght));

			reverse_iterator<const char*> start(reinterpret_cast<char*>(content) + lenght);
			reverse_iterator<const char*> end(reinterpret_cast<char*>(content));

			string formOpenTag("<form");
			string formCloseTag("form>");
			
			auto closeIterator = search(start, end, rbegin(formCloseTag), rend(formCloseTag));
			auto openIterator = search(closeIterator, end, rbegin(formOpenTag), rend(formOpenTag));

			auto fs = (openIterator + formOpenTag.length()).base();
			auto fe = (closeIterator).base();

			return MosMetroResponseParser::GetPostString(string(fs, fe).data());
		});
}
