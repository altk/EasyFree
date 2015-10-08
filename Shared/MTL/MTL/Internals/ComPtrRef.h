#pragma once
#include <roapi.h>
 
namespace MTL
{
	namespace Internals
	{
		template <typename T>
		struct ComPtrRef final
		{
			static_assert(std::is_base_of<IInspectable, T>::value, "T must inherit IInspectable");

			ComPtrRef() = delete;
			ComPtrRef(const ComPtrRef&) = delete;
			ComPtrRef& operator=(const ComPtrRef&) = delete;

			explicit ComPtrRef(T** pointer) noexcept
				: _pointer(pointer)
			{
			}

			ComPtrRef(ComPtrRef&& other) noexcept
				: _pointer(other._pointer)
			{
				other._pointer = nullptr;
			}

			ComPtrRef& operator=(ComPtrRef&& other) noexcept
			{
				if (this != &other)
				{
					_pointer = other._pointer;
					other._pointer = nullptr;
				}
				return *this;
			}

			operator IInspectable**() noexcept
			{
				return reinterpret_cast<IInspectable**>(_pointer);
			}

			operator T**() noexcept
			{
				return _pointer;
			}

			operator void**() noexcept
			{
				return reinterpret_cast<void**>(_pointer);
			}

		private:

			T** _pointer = nullptr;
		};

		template <>
		struct ComPtrRef<IInspectable> final
		{
			ComPtrRef() = delete;
			ComPtrRef(const ComPtrRef&) = delete;
			ComPtrRef& operator=(const ComPtrRef&) = delete;

			explicit ComPtrRef(IInspectable** pointer) noexcept
				: _pointer(pointer)
			{
			}

			ComPtrRef(ComPtrRef&& other) noexcept
				: _pointer(other._pointer)
			{
				other._pointer = nullptr;
			}

			ComPtrRef& operator=(ComPtrRef&& other) noexcept
			{
				if (this != &other)
				{
					_pointer = other._pointer;
					other._pointer = nullptr;
				}
				return *this;
			}

			operator IInspectable**() noexcept
			{
				return _pointer;
			}

			operator void**() noexcept
			{
				return reinterpret_cast<void**>(_pointer);
			}

		private:

			IInspectable** _pointer = nullptr;
		};
	}
}

namespace ABI
{
	namespace Windows
	{
		namespace Foundation
		{
			template <typename TInterface>
			inline STDMETHODIMP GetActivationFactory(HSTRING activatableClassId, MTL::Internals::ComPtrRef<TInterface> factory) noexcept
			{
				return GetActivationFactory(activatableClassId, static_cast<TInterface**>(factory));
			}
		}
	}
}
