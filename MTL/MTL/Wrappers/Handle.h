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
				: _pointer(value)
			{
			}

			Handle(const Handle&) = delete;

			Handle& operator=(const Handle&) = delete;
			
			Handle(Handle&& other) NOEXCEPT
				: _pointer(other.Detach())
			{
			}

			Handle& operator=(Handle&& other) NOEXCEPT
			{
				if (this != other)
				{
					Attach(other.Detach());
				}

				return *this;
			}

			~Handle() NOEXCEPT
			{
				Close();
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
				ASSERT(!*this);

				return &_pointer;
			}

			Pointer* ReleaseAndGetAddressOf() NOEXCEPT
			{
				Close();
				return GetAddressOf();
			}

			Pointer Detach() NOEXCEPT
			{
				Pointer value = _pointer;
				_pointer = Traits::Invalid();
				return value;
			}

			void Reset(Pointer ptr = Traits::Invalid()) NOEXCEPT
			{
				Close();
				_pointer = ptr;
			}

			void Swap(Handle& other) NOEXCEPT
			{
				std::swap(_pointer, other._pointer);
			}
		
		protected:

			void Close() NOEXCEPT
			{
				Traits::Close(_pointer);
			}

		private:

			Pointer _pointer;

		};
	}
}

namespace std
{
	template<typename Traits>
	inline void swap(MTL::Wrappers::Handle<Traits> & left, MTL::Wrappers::Handle<Traits> & right) NOEXCEPT
	{
		left.Swap(right);
	}
}