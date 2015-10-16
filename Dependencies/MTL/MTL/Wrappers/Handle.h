#pragma once
#include <utility>
#include <macro.h>

namespace MTL
{
	namespace Wrappers
	{
		template <typename Traits>
		class Handle
		{
		public:

			using Pointer = typename Traits::Pointer;

			explicit Handle(Pointer value = Traits::Invalid()) NOEXCEPT
				: _pointer(value) { }

			Handle(const Handle&) = delete;

			Handle& operator=(const Handle&) = delete;

			Handle(Handle&& other) NOEXCEPT
				: _pointer(other.Detach()) { }

			Handle& operator=(Handle&& other) NOEXCEPT
			{
				if (this != &other)
				{
					Attach(other.Detach());
				}

				return *this;
			}

			~Handle() NOEXCEPT
			{
				ReleaseInternal();
			}

			explicit operator bool() const NOEXCEPT
			{
				return _pointer != Traits::Invalid();
			}

			Pointer Get() const NOEXCEPT
			{
				return _pointer;
			}

			Pointer* GetAddressOf() NOEXCEPT
			{
				return &_pointer;
			}
			
			void Release() NOEXCEPT
			{
				ReleaseInternal();
			}

			Pointer* ReleaseAndGetAddressOf() NOEXCEPT
			{
				ReleaseInternal();
				return GetAddressOf();
			}

			void Attach(Pointer pointer) NOEXCEPT
			{
				ReleaseInternal();
				_pointer = pointer;
			}

			Pointer Detach() NOEXCEPT
			{
				Pointer value = _pointer;
				_pointer = Traits::Invalid();
				return value;
			}

			void Swap(Handle& other) NOEXCEPT
			{
				std::swap(_pointer, other._pointer);
			}

		protected:

			void ReleaseInternal() NOEXCEPT
			{
				Traits::Release(_pointer);
			}

		private:

			Pointer _pointer;
		};
	}
}

namespace std
{
	template <typename Traits>
	inline void swap(MTL::Wrappers::Handle<Traits>& left, MTL::Wrappers::Handle<Traits>& right) NOEXCEPT
	{
		left.Swap(right);
	}
}
