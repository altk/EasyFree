#pragma once
#include <macro.h>
#include <MTL\Internals\utility.h>
#include <MTL\Internals\ComPtrRef.h>
#include <MTL\Internals\RemoveIUnknown.h>
#include <MTL\Internals\TypeAggregator.h>
#include <MTL\Internals\ConsistencyChecker.h>

namespace MTL
{
	namespace Client
	{
		template <typename T, typename ... Ts>
		class ComPtr final
		{
			friend void swap(ComPtr&, ComPtr&) NOEXCEPT;

			static_assert(Internals::variadic_is_base_of<IUnknown, T, Ts...>::value, "Not all interfaces inherit IUnknown.");
			static_assert(Internals::is_type_set<T, Ts...>::value, "Found duplicate types. You must specify unique types.");

		public:

			ComPtr() NOEXCEPT = default;

			explicit ComPtr(T* defaultInterface) NOEXCEPT
				: _pointer(reinterpret_cast<Internals::TypeAggregator<T, Ts...>*>(defaultInterface)) { }

			ComPtr(const ComPtr& other) NOEXCEPT
				: _pointer(other._pointer)
			{
				InternalAddRef();
			}

			ComPtr(ComPtr&& other) NOEXCEPT
				: _pointer(other._pointer)
			{
				other._pointer = nullptr;
			}

			ComPtr& operator=(const ComPtr& other) NOEXCEPT
			{
				InternalCopy(other);
				return *this;
			}

			ComPtr& operator=(ComPtr&& other) NOEXCEPT
			{
				InternalMove(std::move(other));
				return *this;
			}

			explicit operator bool() const NOEXCEPT
			{
				return nullptr != _pointer;
			}

			Internals::RemoveIUnknown<Internals::TypeAggregator<T, Ts...>>* operator->() const NOEXCEPT
			{
				return Get();
			}

			Internals::ComPtrRef<T> operator&() NOEXCEPT
			{
				return GetAddressOf();
			}

			Internals::RemoveIUnknown<Internals::TypeAggregator<T, Ts...>>* Get() const NOEXCEPT
			{
				using namespace Internals;

				return static_cast<RemoveIUnknown<TypeAggregator<T, Ts...>>*>(_pointer);
			}

			Internals::ComPtrRef<T> GetAddressOf() NOEXCEPT
			{
				ASSERT(!*this);
				auto r = Internals::ComPtrRef<T>(reinterpret_cast<T**>(&_pointer));
				return r;
			}

			Internals::ComPtrRef<T> ReleaseAndGetAddressOf() NOEXCEPT
			{
				InternalRelease();
				return GetAddressOf();
			}

			T* Detach() NOEXCEPT
			{
				Internals::TypeAggregator<T, Ts...>* temp = nullptr;
				std::swap(temp, _pointer);
				return static_cast<T*>(temp);
			}

			void Reset(T* ptr = nullptr) NOEXCEPT
			{
				InternalRelease();
				_pointer = reinterpret_cast<Internals::TypeAggregator<T, Ts...>*>(ptr);
			}

			template <typename U>
			STDMETHODIMP As(Internals::ComPtrRef<U> target) NOEXCEPT
			{
				ASSERT(nullptr != _pointer);

				return (static_cast<T*>(_pointer))->QueryInterface(static_cast<U**>(target));
			}

			void CheckConsistency() const NOEXCEPT
			{
#ifdef _DEBUG
				Internals::CheckConsistency<Internals::TypeAggregator<T, Ts...>, T, Ts...>(_pointer);
#endif
			}

			void Swap(ComPtr& other) NOEXCEPT
			{
				std::swap(_pointer, other._pointer);
			}

		private:

			Internals::TypeAggregator<T, Ts...>* _pointer = nullptr;

			void InternalAddRef() NOEXCEPT
			{
				if (_pointer) static_cast<T*>(_pointer)->AddRef();
			}

			void InternalRelease() NOEXCEPT
			{
				auto temp = _pointer;
				if (temp)
				{
					_pointer = nullptr;
					static_cast<T*>(temp)->Release();
				}
			}

			void InternalCopy(const ComPtr& other) NOEXCEPT
			{
				if (_pointer != other._pointer)
				{
					InternalRelease();
					_pointer = other._pointer;
					InternalAddRef();
				}
			}

			void InternalMove(ComPtr&& other) NOEXCEPT
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
	inline void swap(MTL::Client::ComPtr<TInterfaces...>& lhs, MTL::Client::ComPtr<TInterfaces...>& rhs) NOEXCEPT
	{
		lhs.Swap(rhs);
	}
}
