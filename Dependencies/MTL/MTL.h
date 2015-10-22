#pragma once
#include <roapi.h>
#include <inspectable.h>
#include <array>
#include <utility>
#include <ppltasks.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <macro.h>

namespace MTL
{
	template <typename>
	class ComPtr;

	namespace Internals
	{
		template <typename Interface>
		struct Cloaked : Interface {};

#pragma region IsCloaked

		template <typename Interface>
		struct IsCloaked : std::false_type { };

		template <typename Interface>
		struct IsCloaked<Cloaked<Interface>> : std::true_type { };

#pragma endregion

#pragma region IsBaseOf

		template <typename TBase, typename T, typename ... Ts>
		struct IsBaseOf : std::conditional<std::is_base_of<TBase, T>::value, IsBaseOf<TBase, Ts...>, std::false_type>::type { };

		template <typename TBase, typename T>
		struct IsBaseOf<TBase, T> : std::is_base_of<TBase, T> { };

#pragma endregion

#pragma region IsSame

		template <typename TLhs, typename TRhs, typename ... Ts>
		struct IsSame : std::conditional<std::is_same<TLhs, TRhs>::value, std::true_type, IsSame<TLhs, Ts...>>::type { };

		template <typename TLhs, typename TRhs>
		struct IsSame<TLhs, TRhs> : std::is_same<TLhs, TRhs> { };

#pragma endregion

#pragma region IsTypeSet

		template <typename T, typename ... Ts>
		struct IsTypeSet : std::conditional<IsSame<T, Ts...>::value, std::false_type, IsTypeSet<Ts...>>::type { };

		template <typename T>
		struct IsTypeSet<T> : std::true_type { };

#pragma endregion

#pragma region InterfaceCounter

		template <typename T, typename ... Ts>
		struct InterfaceCounter
		{
			static CONSTEXPR unsigned typesCount = InterfaceCounter<Ts...>::typesCount + static_cast<unsigned>(!IsCloaked<T>::value);
		};

		template <typename T>
		struct InterfaceCounter<T>
		{
			static CONSTEXPR unsigned typesCount = static_cast<unsigned>(!IsCloaked<T>::value);
		};

#pragma endregion

		template <typename ... Ts>
		struct IidsHolder
		{
			static GUID* getIids() NOEXCEPT
			{
				return (new std::array<GUID, sizeof...(Ts)>{__uuidof(Ts)...})->data();
			}
		};

		template <unsigned Counter, typename ... Ts>
		struct CloakedFilter;

		template <unsigned Counter>
		struct CloakedFilter<Counter> : IidsHolder<> { };

		template <unsigned Counter, typename T, typename ... Ts>
		struct CloakedFilter<Counter, T, Ts...> :
				std::conditional<Counter == sizeof...(Ts) + 1,
								 IidsHolder<T, Ts...>,
								 typename std::conditional<IsCloaked<T>::value, CloakedFilter<Counter, Ts...>, CloakedFilter<Counter + 1, Ts..., T>>::type>::type { };

		template <typename T, typename ... Ts>
		struct IidsExtractor : CloakedFilter<0, T, Ts...> { };

#pragma region FunctionTraits

		template <class>
		struct FunctionTraits;

		template <class TResult, class... TArgs>
		struct FunctionTraits<TResult(*)(TArgs ...)> : FunctionTraits<TResult(TArgs ...)> {};

		template <class TResult, class... TArgs>
		struct FunctionTraits<TResult(TArgs ...)>
		{
			using ReturnType = TResult;
			using TypesTuple = std::tuple<TArgs...>;

			static CONSTEXPR size_t arity = sizeof...(TArgs);

			template <size_t TIndex>
			struct Argument
			{
				static_assert(TIndex < arity, "error: invalid parameter index.");
				using type = typename std::tuple_element<TIndex, std::tuple<TArgs...>>::type;
			};
		};

		template <class TClass, class TResult, class... TArgs>
		struct FunctionTraits<TResult(TClass::*)(TArgs ...)> : FunctionTraits<TResult(TArgs ...)> {};

		template <class TClass, class TResult, class... TArgs>
		struct FunctionTraits<TResult(TClass::*)(TArgs ...) const> : FunctionTraits<TResult(TArgs ...)> {};

		template <class TClass, class TResult>
		struct FunctionTraits<TResult(TClass::*)> : FunctionTraits<TResult()> {};

#pragma endregion 

#pragma region RemoveIUnknown

		template <typename TInterface>
		struct RemoveIUnknown abstract : TInterface
		{
			static_assert(std::is_base_of<IUnknown, TInterface>::value, "TInterface must inherit IUnknown");
		private:
			STDMETHODIMP_(ULONG) AddRef();
			STDMETHODIMP_(ULONG) Release();
			STDMETHODIMP QueryTInterface(IID, void**);
		};

#pragma endregion

#pragma region ComPtrRef

		template <typename TClass>
		struct ComPtrRef final
		{
			ComPtrRef() = delete;
			ComPtrRef& operator=(const ComPtrRef&) = delete;
			ComPtrRef& operator=(ComPtrRef&& other) = delete;

			ComPtrRef(const ComPtrRef& other) NOEXCEPT
				: _pointer(other._pointer) {}

			ComPtrRef(ComPtrRef&& other) NOEXCEPT
				: _pointer(std::move(other._pointer)) {}

			explicit ComPtrRef(ComPtr<TClass>& pointer) NOEXCEPT
				: _pointer(pointer) { }

			operator IUnknown**() NOEXCEPT
			{
				return reinterpret_cast<IUnknown**>(&_pointer._pointer);
			}

			operator IInspectable**() NOEXCEPT
			{
				return reinterpret_cast<IInspectable**>(&_pointer._pointer);
			}

			operator TClass**() NOEXCEPT
			{
				return &_pointer._pointer;
			}

			operator void**() NOEXCEPT
			{
				return reinterpret_cast<void**>(&_pointer._pointer);
			}

		private:

			ComPtr<TClass>& _pointer;
		};

#pragma endregion

#pragma region Handle

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

#pragma endregion
	}

#pragma region ComClass

	//TODO âûíåñòè ïîääåðæêó ïîäñ÷åòà ññûëîê â îòäåëüíûé êëàññ
	template <typename TDefaultInterface,
			  typename ... TInterfaces>
	class ComClass abstract : public TDefaultInterface,
							  public TInterfaces...
	{
		static_assert(Internals::IsBaseOf<IUnknown, TDefaultInterface, TInterfaces...>::value, "Not all interfaces inherit IUnknown.");
		static_assert(Internals::IsTypeSet<TDefaultInterface, TInterfaces...>::value, "Found duplicate types. You must specify unique types.");

	public:
		STDMETHODIMP_(ULONG) AddRef() NOEXCEPT override final
		{
			return InterlockedIncrement(&_counter);
		}

		STDMETHODIMP_(ULONG) Release() NOEXCEPT override final
		{
			auto const remaining = InterlockedDecrement(&_counter);
			if (0 == remaining)
			{
				delete this;
			}
			return remaining;
		}

		STDMETHODIMP QueryInterface(const GUID& guid, void** result) NOEXCEPT override final
		{
			if (guid == __uuidof(TDefaultInterface) ||
				guid == __uuidof(IUnknown) ||
				guid == __uuidof(IInspectable))
			{
				*result = this;
			}
			else
			{
				*result = QueryInterfaceImpl<TInterfaces...>(guid);
			}

			if (nullptr == *result) return E_NOINTERFACE;

			static_cast<IUnknown*>(*result)->AddRef();
			return S_OK;
		}

	protected:
		template <typename U, typename ... Us>
		void* QueryInterfaceImpl(const GUID& guid) NOEXCEPT
		{
			using namespace Internals;

			if (IsCloaked<U>::value || guid != __uuidof(U))
			{
				return QueryInterfaceImpl<Us...>(guid);
			}
			return static_cast<U*>(this);
		}

		template <int = 0>
		void* QueryInterfaceImpl(const GUID&) const NOEXCEPT
		{
			return nullptr;
		}

	private:
		volatile ULONG _counter = 1;
	};

#pragma endregion

#pragma region RuntimeClass

	template <typename TDefaultInterface, typename ... TInterfaces>
	class RuntimeClass abstract : public ComClass<TDefaultInterface, TInterfaces...>
	{
	public:
		STDMETHODIMP GetIids(ULONG* count, GUID** array) NOEXCEPT override final
		{
			using namespace Internals;

			*count = InterfaceCounter<TDefaultInterface, TInterfaces...>::typesCount;
			*array = const_cast<GUID*>(IidsExtractor<TDefaultInterface, TInterfaces...>::getIids());
			if (nullptr == *array)
			{
				return E_OUTOFMEMORY;
			}
			return S_OK;
		}

		STDMETHODIMP GetTrustLevel(TrustLevel* trustLevel) NOEXCEPT override final
		{
			*trustLevel = BaseTrust;
			return S_OK;
		}
	};

#pragma endregion

#pragma region ActivationFactory

	template <typename TClass>
	class ActivationFactory final : public RuntimeClass<IActivationFactory>
	{
	public:
		STDMETHODIMP ActivateInstance(IInspectable** instance) NOEXCEPT override final
		{
			*instance = new(std::nothrow) TClass();
			if (nullptr == *instance)
			{
				return E_OUTOFMEMORY;
			}
			return S_OK;
		}

		STDMETHODIMP GetRuntimeClassName(HSTRING*) NOEXCEPT override final
		{
			return E_ILLEGAL_METHOD_CALL;
		}
	};

#pragma endregion

#pragma region ComPtr

	template <typename TClass>
	class ComPtr final
	{
		static_assert(std::is_base_of<IUnknown, TClass>::value, "Not all interfaces inherit IUnknown.");

		friend void swap(ComPtr&, ComPtr&) NOEXCEPT;
		friend struct Internals::ComPtrRef<TClass>;

	public:

		ComPtr() NOEXCEPT
			: _pointer(nullptr) { };

		explicit ComPtr(TClass* defaultInterface) NOEXCEPT
			: _pointer(defaultInterface) { }

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

		Internals::RemoveIUnknown<TClass>* operator->() const NOEXCEPT
		{
			return Get();
		}

		Internals::ComPtrRef<TClass> operator&() NOEXCEPT
		{
			return GetAddressOf();
		}

		Internals::RemoveIUnknown<TClass>* Get() const NOEXCEPT
		{
			using namespace Internals;

			return static_cast<RemoveIUnknown<TClass>*>(_pointer);
		}

		Internals::ComPtrRef<TClass> GetAddressOf() NOEXCEPT
		{
			ASSERT(!*this);
			return Internals::ComPtrRef<TClass>(*this);
		}

		void Release() NOEXCEPT
		{
			InternalRelease();
		}

		Internals::ComPtrRef<TClass> ReleaseAndGetAddressOf() NOEXCEPT
		{
			InternalRelease();
			return GetAddressOf();
		}

		void Attach(TClass* ptr) NOEXCEPT
		{
			InternalRelease();
			_pointer = ptr;
			InternalAddRef();
		}

		TClass* Detach() NOEXCEPT
		{
			TClass* temp = _pointer;
			_pointer = nullptr;
			return static_cast<TClass*>(temp);
		}

		template <typename U>
		STDMETHODIMP As(Internals::ComPtrRef<U> target) NOEXCEPT
		{
			ASSERT(nullptr != _pointer);

			return _pointer->QueryInterface(static_cast<U**>(target));
		}

		void Swap(ComPtr& other) NOEXCEPT
		{
			std::swap(_pointer, other._pointer);
		}

	private:

		TClass* _pointer = nullptr;

		void InternalAddRef() NOEXCEPT
		{
			if (_pointer)
			{
				_pointer->AddRef();
			}
		}

		void InternalRelease() NOEXCEPT
		{
			auto temp = _pointer;
			if (temp)
			{
				_pointer = nullptr;
				temp->Release();
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

#pragma endregion

#pragma region HStringTraits

	struct HStringTraits final
	{
		using Pointer = HSTRING;

		static Pointer Invalid() NOEXCEPT
		{
			return nullptr;
		}

		static void Release(Pointer value) NOEXCEPT
		{
			VERIFY_SUCCEEDED(WindowsDeleteString(value));
		}
	};

#pragma endregion

#pragma region HString

	// TODO íàïèñàòü ïåðåãðóçêó îïåðàòîðà operator&
	class HString final : public Internals::Handle<HStringTraits>
	{
	public:
		explicit HString(Pointer pointer = HStringTraits::Invalid()) NOEXCEPT
			: Handle<HStringTraits>(pointer) { }

		HString(const wchar_t* const string, unsigned const length) NOEXCEPT
		{
			VERIFY_SUCCEEDED(WindowsCreateString(string, length, GetAddressOf()));
		}

		template <unsigned Length>
		explicit HString(const wchar_t (&string)[Length]) NOEXCEPT
			: HString(string, Length - 1) { }

		HString(const HString& other) NOEXCEPT
		{
			VERIFY_SUCCEEDED(WindowsDuplicateString(other.Get(), GetAddressOf()));
		}

		HString(HString&& other) NOEXCEPT
			: Handle<HStringTraits>(other.Detach()) { }

		HString& operator=(const HString& other) NOEXCEPT
		{
			if (this != &other)
			{
				ReleaseInternal();
				VERIFY_SUCCEEDED(WindowsDuplicateString(other.Get(), GetAddressOf()));
			}
			return *this;
		}

		HString& operator=(HString&& other) NOEXCEPT
		{
			if (this != &other)
			{
				Handle<HStringTraits>::operator=(std::move(other));
			}
			return *this;
		}

		friend bool operator==(const HString& lhs, const HString& rhs) NOEXCEPT
		{
			INT32 compareResult;
			WindowsCompareStringOrdinal(lhs.Get(), rhs.Get(), &compareResult);
			return compareResult == 0;
		}

		friend bool operator!=(const HString& lhs, const HString& rhs) NOEXCEPT
		{
			return !(lhs == rhs);
		}

		HString Substring(unsigned start) const NOEXCEPT
		{
			HString result;
			VERIFY_SUCCEEDED(WindowsSubstring(Get(), start, result.GetAddressOf()));
			return result;
		}

		const wchar_t* GetRawBuffer() const NOEXCEPT
		{
			return WindowsGetStringRawBuffer(Get(), nullptr);
		}

		const wchar_t* GetRawBuffer(unsigned* length) const NOEXCEPT
		{
			return WindowsGetStringRawBuffer(Get(), length);
		}

		unsigned Size() const NOEXCEPT
		{
			return WindowsGetStringLen(Get());
		}

		bool Empty() const NOEXCEPT
		{
			return 0 == WindowsIsStringEmpty(Get());
		}
	};

#pragma endregion

#pragma region HStringReference

	class HStringReference final
	{
	public:
		HStringReference(const wchar_t* const value, unsigned length) NOEXCEPT
		{
			VERIFY_SUCCEEDED(WindowsCreateStringReference(value, length, &_stringHeader, &_string));
		}

		template <unsigned Length>
		explicit HStringReference(const wchar_t (&string)[Length]) NOEXCEPT
			: HStringReference(string, Length - 1) { }

		HStringReference(const HStringReference&) = delete;

		HStringReference& operator=(const HStringReference&) = delete;

		void* operator new(size_t) = delete;

		void* operator new[](size_t) = delete;

		void operator delete(void*) = delete;

		void operator delete[](void*) = delete;

		HSTRING Get() const NOEXCEPT
		{
			return _string;
		}

	private:
		HSTRING _string;
		HSTRING_HEADER _stringHeader;
	};

#pragma endregion

#pragma region IteratorAdapter

	template <class TIterator>
	class IteratorAdapter final : public RuntimeClass<ABI::Windows::Foundation::Collections::IIterator<typename TIterator::value_type>>
	{
	public:

		IteratorAdapter(TIterator&& begin, TIterator&& end) NOEXCEPT
			: _begin(std::forward<TIterator>(begin))
			, _end(std::forward<TIterator>(end))
		{
		}

		STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override
		{
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

#pragma endregion

#pragma region IterableAdapter

	template <class TCollection>
	class IterableAdapter final : public RuntimeClass<ABI::Windows::Foundation::Collections::IIterable<typename TCollection::value_type>>
	{
	public:

		explicit IterableAdapter(TCollection&& collection) NOEXCEPT
			: _collection(std::forward<TCollection>(collection))
		{
		}

		STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override
		{
			*className = HString(L"MTL.IterableAdapter<`1>").Detach();
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

#pragma endregion

	namespace Internals
	{
		template <typename TDelegateInterface,
				  typename TCallback,
				  typename ... TArgs>
		class InvokeHelper final : public ComClass<TDelegateInterface>
		{
		public:
			explicit InvokeHelper(TCallback&& callback) NOEXCEPT
				: _callback(std::forward<TCallback>(callback)) { }

			InvokeHelper(const InvokeHelper& other) NOEXCEPT
				: _callback(other._callback) { }

			InvokeHelper(InvokeHelper&& other) NOEXCEPT
				: _callback(std::move(other._callback)) { }

			InvokeHelper& operator=(const InvokeHelper& other) NOEXCEPT
			{
				if (this != &other)
				{
					_callback = other._callback;
				}
				return *this;
			}

			InvokeHelper& operator=(InvokeHelper&& other) NOEXCEPT
			{
				if (this != &other)
				{
					_callback = std::move(other._callback);
				}
				return *this;
			}

			STDMETHODIMP Invoke(TArgs ... args) NOEXCEPT override
			{
				return _callback(std::forward<TArgs>(args)...);
			}

		private:
			TCallback _callback;
		};

		template <typename TDelegateInterface,
				  typename TCallback,
				  typename ... TArgs>
		inline InvokeHelper<TDelegateInterface, TCallback, TArgs...> ÑreateInvokeHelper(TCallback&& callback,
																						std::tuple<TArgs...>) NOEXCEPT
		{
			return InvokeHelper<TDelegateInterface, TCallback, TArgs...>(std::forward<TCallback>(callback));
		}
	}

	template <typename TClass>
	inline ComPtr<TClass> CreateComPtr(TClass* ptr) NOEXCEPT
	{
		return ComPtr<TClass>(ptr);
	}

	template <typename TInterface>
	inline STDMETHODIMP GetActivationFactory(HSTRING activatableClassId,
											 Internals::ComPtrRef<TInterface> factory) NOEXCEPT
	{
		return RoGetActivationFactory(activatableClassId,
									  __uuidof(TInterface),
									  static_cast<void**>(factory));
	}

	template <typename TDelegateInterface,
			  typename TCallback>
	inline ComPtr<TDelegateInterface> CreateCallback(TCallback&& callback) NOEXCEPT
	{
		using namespace Internals;

		static_assert(std::is_base_of<IUnknown, TDelegateInterface>::value && !std::is_base_of<IInspectable, TDelegateInterface>::value, "Delegates objects must be 'IUnknown' base and not 'IInspectable'");

		using tuple = typename FunctionTraits<decltype(&TDelegateInterface::Invoke)>::TypesTuple;

		auto helper = ÑreateInvokeHelper<TDelegateInterface>(std::forward<TCallback>(callback), tuple());

		using helperType = decltype(helper);

		return ComPtr<TDelegateInterface>(new helperType(std::move(helper)));
	};

	template <typename TArgument>
	static auto GetTask(ABI::Windows::Foundation::IAsyncOperation<TArgument>* asyncOperation) NOEXCEPT ->
	concurrency::task<typename ABI::Windows::Foundation::Internal::GetAbiType<typename ABI::Windows::Foundation::IAsyncOperation<TArgument>::TResult_complex>::type>
	{
		using namespace concurrency;
		using namespace ABI::Windows::Foundation::Internal;
		using namespace ABI::Windows::Foundation;

		using TResult = typename GetAbiType<typename IAsyncOperation<TArgument>::TResult_complex>::type;

		task_completion_event<TResult> taskCompletitionEvent;
		auto callback = CreateCallback<IAsyncOperationCompletedHandler<TArgument>>([taskCompletitionEvent](IAsyncOperation<TArgument>* operation, AsyncStatus status)-> HRESULT
																				   {
																					   TResult result;
																					   operation->GetResults(&result);
																					   taskCompletitionEvent.set(result);
																					   return S_OK;
																				   });
		asyncOperation->put_Completed(callback.Get());
		return task<TResult>(taskCompletitionEvent);
	}

	template <typename TArgument, typename TProgress>
	static auto GetTask(ABI::Windows::Foundation::IAsyncOperationWithProgress<TArgument, TProgress>* asyncOperation) NOEXCEPT ->
	concurrency::task<typename ABI::Windows::Foundation::Internal::GetAbiType<typename ABI::Windows::Foundation::IAsyncOperationWithProgress<TArgument, TProgress>::TResult_complex>::type>
	{
		using namespace concurrency;
		using namespace ABI::Windows::Foundation::Internal;
		using namespace ABI::Windows::Foundation;

		using TResult = typename GetAbiType<typename IAsyncOperationWithProgress<TArgument, TProgress>::TResult_complex>::type;

		task_completion_event<TResult> taskCompletitionEvent;
		auto callback = CreateCallback<IAsyncOperationWithProgressCompletedHandler<TArgument, TProgress>>([taskCompletitionEvent](IAsyncOperationWithProgress<TArgument, TProgress>* operation, AsyncStatus status)-> HRESULT
																										  {
																											  TResult result;
																											  operation->GetResults(&result);
																											  taskCompletitionEvent.set(result);
																											  return S_OK;
																										  });
		asyncOperation->put_Completed(callback.Get());
		return task<TResult>(taskCompletitionEvent);
	}
}

namespace std
{
	template <typename ... TInterfaces>
	inline void swap(MTL::ComPtr<TInterfaces...>& lhs,
					 MTL::ComPtr<TInterfaces...>& rhs) NOEXCEPT
	{
		lhs.Swap(rhs);
	}
}
