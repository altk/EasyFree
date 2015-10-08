#pragma once
#include <MTL\Internals\utility.h>
#include <MTL\Internals\ComPtrRef.h>
#include <MTL\Internals\RemoveIUnknown.h>
#include <MTL\Internals\TypeAggregator.h>
#include <MTL\Internals\ConsistencyChecker.h>
#include <macro.h>

namespace MTL
{
	namespace Client
	{
		template <typename T, typename ... Ts>
		class ComPtr final
		{
			friend void swap(ComPtr&, ComPtr&);

			static_assert(Internals::variadic_is_base_of<IUnknown, T, Ts...>::value, "Not all interfaces inherit IUnknown.");
			static_assert(Internals::is_type_set<T, Ts...>::value, "Found duplicate types. You must specify unique types.");

		public:

			ComPtr() noexcept = default;

			explicit ComPtr(T* defaultInterface) noexcept
				: _pointer(reinterpret_cast<Internals::TypeAggregator<T, Ts...>*>(defaultInterface)) { }

			ComPtr(const ComPtr& other) noexcept
				: _pointer(other._pointer)
			{
				InternalAddRef();
			}

			ComPtr(ComPtr&& other) noexcept
				: _pointer(other._pointer)
			{
				other._pointer = nullptr;
			}

			ComPtr& operator=(const ComPtr& other) noexcept
			{
				InternalCopy(other);
				return *this;
			}

			ComPtr& operator=(ComPtr&& other) noexcept
			{
				InternalMove(std::move(other));
				return *this;
			}

			explicit operator bool() const noexcept
			{
				return nullptr != _pointer;
			}

			Internals::RemoveIUnknown<Internals::TypeAggregator<T, Ts...>>* operator->() const noexcept
			{
				return Get();
			}

			Internals::ComPtrRef<T> operator&() noexcept
			{
				return GetAddressOf();
			}

			Internals::RemoveIUnknown<Internals::TypeAggregator<T, Ts...>>* Get() const noexcept
			{
				using namespace Internals;

				return static_cast<RemoveIUnknown<TypeAggregator<T, Ts...>>*>(_pointer);
			}

			Internals::ComPtrRef<T> GetAddressOf() noexcept
			{
				ASSERT(!*this);
				auto r = Internals::ComPtrRef<T>(reinterpret_cast<T**>(&_pointer));
				return r;
			}

			Internals::ComPtrRef<T> ReleaseAndGetAddressOf() noexcept
			{
				InternalRelease();
				return GetAddressOf();
			}

			T* Detach() noexcept
			{
				Internals::TypeAggregator<T, Ts...>* temp = nullptr;
				std::swap(temp, _pointer);
				return static_cast<T*>(temp);
			}

			void Reset(T* ptr = nullptr) noexcept
			{
				InternalRelease();
				_pointer = reinterpret_cast<Internals::TypeAggregator<T, Ts...>*>(ptr);
			}

			template <typename U>
			STDMETHODIMP As(Internals::ComPtrRef<U> target) noexcept
			{
				ASSERT(nullptr != _pointer);

				return (static_cast<T*>(_pointer))->QueryInterface(static_cast<U**>(target));
			}

			void CheckConsistency() const noexcept
			{
#ifdef _DEBUG
				Internals::CheckConsistency<Internals::TypeAggregator<T, Ts...>, T, Ts...>(_pointer);
#endif
			}

			void Swap(ComPtr& other) noexcept
			{
				std::swap(_pointer, other._pointer);
			}

		private:

			Internals::TypeAggregator<T, Ts...>* _pointer = nullptr;

			void InternalAddRef() noexcept
			{
				if (_pointer) static_cast<T*>(_pointer)->AddRef();
			}

			void InternalRelease() noexcept
			{
				auto temp = _pointer;
				if (temp)
				{
					_pointer = nullptr;
					static_cast<T*>(temp)->Release();
				}
			}

			void InternalCopy(const ComPtr& other) noexcept
			{
				if (_pointer != other._pointer)
				{
					InternalRelease();
					_pointer = other._pointer;
					InternalAddRef();
				}
			}

			void InternalMove(ComPtr&& other) noexcept
			{
				if (_pointer != other._pointer)
				{
					InternalRelease();
					_pointer = other._pointer;
					other._pointer = nullptr;
				}
			}
		};
	}
}

namespace std
{
	template <typename ... TInterfaces>
	inline void swap(MTL::Client::ComPtr<TInterfaces...>& lhs, MTL::Client::ComPtr<TInterfaces...>& rhs) noexcept
	{
		lhs.Swap(rhs);
	}
}
