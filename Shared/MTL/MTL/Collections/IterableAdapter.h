#pragma once
#include <windows.foundation.collections.h>
#include <MTL\Implements\RuntimeClass.h>
#include <MTL\collections\IteratorAdapter.h>

namespace MTL
{
	namespace Collections
	{
		template <class TCollection>
		class IterableAdapter final : public Implements::RuntimeClass<ABI::Windows::Foundation::Collections::IIterable<typename TCollection::value_type>>
		{
		public:

			explicit IterableAdapter(TCollection&& collection) noexcept
				: _collection(std::forward<TCollection>(collection))
			{
			}

			STDMETHODIMP GetRuntimeClassName(HSTRING* className) noexcept override
			{
				using namespace Wrappers;

				*className = HString(L"mytarget.IterableAdapter<`1>").Detach();
				return S_OK;
			}

			STDMETHODIMP First(ABI::Windows::Foundation::Collections::IIterator<typename TCollection::value_type>** first) override
			{
				*first = new (std::nothrow) IteratorAdapter<typename TCollection::iterator>(_collection.begin(), _collection.end());
				if (nullptr == *first)
				{
					return E_OUTOFMEMORY;
				}
				return S_OK;
			}

		private:
			TCollection _collection;
		};
	}
}