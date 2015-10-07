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

			explicit Handle(Pointer value = Traits::Invalid()) noexcept
				: _pointer(value)
			{
			}

			Handle(const Handle&) = delete;

			Handle& operator=(const Handle&) = delete;
			
			Handle(Handle&& other) noexcept
				: _pointer(other.Detach())
			{
			}

			Handle& operator=(Handle&& other) noexcept
			{
				if (this != other)
				{
					Attach(other.Detach());
				}

				return *this;
			}

			~Handle() noexcept
			{
				Close();
			}

			explicit operator bool() const noexcept
			{
				return _pointer != Traits::Invalid();
			}

			Pointer Get() const noexcept
			{
				return _pointer;
			}

			Pointer* GetAddressOf() noexcept
			{
				ASSERT(!*this);

				return &_pointer;
			}

			Pointer* ReleaseAndGetAddressOf() noexcept
			{
				Close();
				return GetAddressOf();
			}

			Pointer Detach() noexcept
			{
				Pointer value = _pointer;
				_pointer = Traits::Invalid();
				return value;
			}

			void Reset(Pointer ptr = Traits::Invalid()) noexcept
			{
				Close();
				_pointer = ptr;
			}

			void Swap(Handle& other) noexcept
			{
				std::swap(_pointer, other._pointer);
			}
		
		protected:

			void Close() noexcept
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
	inline void swap(MTL::Wrappers::Handle<Traits> & left, MTL::Wrappers::Handle<Traits> & right) noexcept
	{
		left.Swap(right);
	}
}