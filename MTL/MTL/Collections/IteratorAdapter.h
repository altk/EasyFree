#pragma once
#include <windows.foundation.collections.h>
#include <macro.h>
#include <MTL\Implements\RuntimeClass.h>

namespace MTL
{
	namespace Collections
	{
		template <class TIterator>
		class IteratorAdapter final : public Implements::RuntimeClass<ABI::Windows::Foundation::Collections::IIterator<typename TIterator::value_type>>
		{
		public:

			IteratorAdapter(TIterator&& begin, TIterator&& end) NOEXCEPT
				: _begin(std::forward<TIterator>(begin))
				, _end(std::forward<TIterator>(end))
			{
			}

			STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override
			{
				using namespace Wrappers;

				*className = HString(L"mytarget.IteratorAdapter<`1>").Detach();
				return S_OK;
			}

			STDMETHODIMP get_Current(typename TIterator::value_type* current) NOEXCEPT override
			{
				*current = *_begin;
				return S_OK;
			}

			STDMETHODIMP get_HasCurrent(boolean* hasCurrent) NOEXCEPT override
			{
				*hasCurrent = _begin != _end;
				return S_OK;
			}

			STDMETHODIMP MoveNext(boolean* hasCurrent) NOEXCEPT override
			{
				if (_begin != _end)
				{
					++_begin;
					*hasCurrent = _begin != _end;
				}
				else
				{
					*hasCurrent = false;
				}
				return S_OK;
			}

		private:
			TIterator _begin;
			TIterator _end;
		};
	}
}
