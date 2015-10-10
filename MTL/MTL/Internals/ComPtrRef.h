#pragma once
#include <roapi.h>
#include <macro.h>

namespace MTL
{
	namespace Internals
	{
		template <typename T>
		struct ComPtrRef final
		{
			ComPtrRef() = delete;
			ComPtrRef(const ComPtrRef&) = delete;
			ComPtrRef& operator=(const ComPtrRef&) = delete;

			explicit ComPtrRef(T** pointer) NOEXCEPT
				: _pointer(pointer)
			{
			}

			ComPtrRef(ComPtrRef&& other) NOEXCEPT
				: _pointer(other._pointer)
			{
				other._pointer = nullptr;
			}

			ComPtrRef& operator=(ComPtrRef&& other) NOEXCEPT
			{
				if (this != &other)
				{
					_pointer = other._pointer;
					other._pointer = nullptr;
				}
				return *this;
			}

			operator IUnknown**() NOEXCEPT
			{
				return reinterpret_cast<IUnknown**>(_pointer);
			}

			operator IInspectable**() NOEXCEPT
			{
				return reinterpret_cast<IInspectable**>(_pointer);
			}

			operator T**() NOEXCEPT
			{
				return _pointer;
			}

			operator void**() NOEXCEPT
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

			explicit ComPtrRef(IInspectable** pointer) NOEXCEPT
				: _pointer(pointer)
			{
			}

			ComPtrRef(ComPtrRef&& other) NOEXCEPT
				: _pointer(other._pointer)
			{
				other._pointer = nullptr;
			}

			ComPtrRef& operator=(ComPtrRef&& other) NOEXCEPT
			{
				if (this != &other)
				{
					_pointer = other._pointer;
					other._pointer = nullptr;
				}
				return *this;
			}

			operator IInspectable**() NOEXCEPT
			{
				return _pointer;
			}

			operator void**() NOEXCEPT
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
			inline STDMETHODIMP GetActivationFactory(HSTRING activatableClassId, MTL::Internals::ComPtrRef<TInterface> factory) NOEXCEPT
			{
				return GetActivationFactory(activatableClassId, static_cast<TInterface**>(factory));
			}
		}
	}
}
