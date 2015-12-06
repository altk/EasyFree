#pragma once
#include <array>
#include <utility>
#include <tuple>
#include <ppltasks.h>
#include <inspectable.h>
#include <roapi.h>
#include <asyncinfo.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <macro.h>
#include <sstream>
#include <iomanip>

namespace MTL
{
	template <typename>
	class ComPtr;

	inline void Check(HRESULT hr);

	namespace Internals
	{
		template <typename Traits>
		class Handle;

#pragma region Cloaked

		template <typename Interface>
		struct Cloaked : Interface {};

#pragma endregion

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

#pragma region IidsHolder

		template <typename ... Ts>
		struct IidsHolder
		{
			static GUID* getIids() NOEXCEPT
			{
				return (new std::array<GUID, sizeof...(Ts)>{__uuidof(Ts)...})->data();
			}
		};

#pragma endregion

#pragma region CloakedFilter

		template <unsigned Counter, typename ... Ts>
		struct CloakedFilter;

		template <unsigned Counter>
		struct CloakedFilter<Counter> : IidsHolder<> { };

		template <unsigned Counter, typename T, typename ... Ts>
		struct CloakedFilter<Counter, T, Ts...> :
				std::conditional<Counter == sizeof...(Ts) + 1,
								 IidsHolder<T, Ts...>,
								 typename std::conditional<IsCloaked<T>::value, CloakedFilter<Counter, Ts...>, CloakedFilter<Counter + 1, Ts..., T>>::type>::type { };

#pragma endregion

#pragma region IidsExtractor

		template <typename T, typename ... Ts>
		struct IidsExtractor : CloakedFilter<0, T, Ts...> { };

#pragma endregion

#pragma region FunctionTraits

		template <typename TFunction>
		struct FunctionTraits;

		template <typename TResult, typename... TArgs>
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

		template <typename TResult, typename... TArgs>
		struct FunctionTraits<TResult(*)(TArgs ...)> : FunctionTraits<TResult(TArgs ...)> {};

		template <typename TClass, typename TResult, typename... TArgs>
		struct FunctionTraits<TResult(TClass::*)(TArgs ...)> : FunctionTraits<TResult(TArgs ...)> {};

		template <typename TClass, typename TResult, typename... TArgs>
		struct FunctionTraits<TResult(TClass::*)(TArgs ...) const> : FunctionTraits<TResult(TArgs ...)> {};

#ifdef _M_IX86
		template <typename TResult, typename... TArgs>
		struct FunctionTraits<TResult(__stdcall *)(TArgs ...)> : FunctionTraits<TResult(TArgs ...)> {};

		template <typename TClass, typename TResult, typename... TArgs>
		struct FunctionTraits<TResult(__stdcall TClass::*)(TArgs ...)> : FunctionTraits<TResult(TArgs ...)> {};

		template <typename TClass, typename TResult, typename... TArgs>
		struct FunctionTraits<TResult(__stdcall TClass::*)(TArgs ...) const> : FunctionTraits<TResult(TArgs ...)> {};
#endif

#pragma endregion 

#pragma region RemoveIUnknown

		template <typename TInterface>
		struct
				NOVTABLE RemoveIUnknown abstract : TInterface
		{
			static_assert(std::is_base_of<IUnknown, TInterface>::value, "TInterface must inherit IUnknown");

			operator TInterface*() NOEXCEPT
			{
				return this;
			}

		private:
			STDMETHODIMP_(ULONG) AddRef();
			STDMETHODIMP_(ULONG) Release();
			STDMETHODIMP QueryTInterface(IID, void**);
		};

#pragma endregion

#pragma region ComPtrRef

		template <typename TClass>
		class ComPtrRef final
		{
			friend class ComPtr<TClass>;
		public:

			ComPtrRef(const ComPtrRef& other)
				: _pointer(other._pointer) {}

			ComPtrRef(ComPtrRef&& other)
				: _pointer(other._pointer) {}

			ComPtrRef& operator=(const ComPtrRef&) = delete;

			ComPtrRef& operator=(ComPtrRef&&) = delete;

			template <typename TResult>
			operator TResult**() const NOEXCEPT
			{
				return ConvertTo<TResult>();
			}

			operator TClass**() const NOEXCEPT
			{
				return _pointer;
			}

			operator void**() const NOEXCEPT
			{
				return reinterpret_cast<void**>(_pointer);
			}

		private:
			explicit ComPtrRef(TClass** pointer) NOEXCEPT
				: _pointer(pointer) { }

			TClass** _pointer;

			template <typename TResult>
			IUnknown** ConvertTo(typename std::enable_if<std::is_base_of<IUnknown, TResult>::value && std::is_base_of<IInspectable, TResult>::value == false>::type* = nullptr) const NOEXCEPT
			{
				return reinterpret_cast<IUnknown**>(_pointer);
			}

			template <typename TResult>
			IInspectable** ConvertTo(typename std::enable_if<std::is_base_of<IInspectable, TResult>::value>::type* = nullptr) const NOEXCEPT
			{
				return reinterpret_cast<IInspectable**>(_pointer);
			}
		};

#pragma endregion

#pragma region HandleRef

		template <typename Traits>
		class HandleRef final
		{
		public:
			using Pointer = typename Traits::Pointer;

			explicit HandleRef(Handle<Traits>& reference) NOEXCEPT
				: _reference(reference) {}

			HandleRef(const HandleRef& other) NOEXCEPT
				: _reference(other._reference) {}

			HandleRef(HandleRef&& other) NOEXCEPT
				: _reference(std::move(other._reference)) { }

			HandleRef& operator=(const HandleRef&) = delete;

			HandleRef& operator=(HandleRef&&) = delete;

			operator Pointer*() NOEXCEPT
			{
				return &_reference._pointer;
			}

			operator Handle<Traits>*() NOEXCEPT
			{
				return &_reference;
			}

		private:
			Handle<Traits>& _reference;
		};

#pragma endregion

#pragma region Handle

		template <typename Traits>
		class Handle
		{
			friend class HandleRef<Traits>;
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

			HandleRef<Traits> operator&() NOEXCEPT
			{
				return HandleRef<Traits>(*this);
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

#pragma region AsyncOperationTaskHelper

		template <typename TArgument, bool = std::is_base_of<IUnknown, std::remove_pointer_t<TArgument>>::value>
		struct AsyncOperationTaskHelper;

		template <typename TArgument>
		struct AsyncOperationTaskHelper<TArgument, true> final
		{
			using TResult = ComPtr<std::remove_pointer_t<TArgument>>;
		};

		template <typename TArgument>
		struct AsyncOperationTaskHelper<TArgument, false> final
		{
			using TResult = TArgument;
		};

#pragma endregion

#pragma region AsynOperationHelper

		template <typename TAsyncOperation>
		struct AsynOperationHelper;

		template <typename TArgument>
		struct AsynOperationHelper<ABI::Windows::Foundation::IAsyncOperation<TArgument>> final
		{
			using TOperation = ABI::Windows::Foundation::IAsyncOperation<TArgument>;
			using TResult = typename AsyncOperationTaskHelper<typename ABI::Windows::Foundation::Internal::GetAbiType<typename TOperation::TResult_complex>::type>::TResult;
			using THandler = ABI::Windows::Foundation::IAsyncOperationCompletedHandler<TArgument>;
		};

		template <typename TArgument>
		struct AsynOperationHelper<RemoveIUnknown<ABI::Windows::Foundation::IAsyncOperation<TArgument>>> final
		{
			using TOperation = ABI::Windows::Foundation::IAsyncOperation<TArgument>;
			using TResult = typename AsyncOperationTaskHelper<typename ABI::Windows::Foundation::Internal::GetAbiType<typename TOperation::TResult_complex>::type>::TResult;
			using THandler = ABI::Windows::Foundation::IAsyncOperationCompletedHandler<TArgument>;
		};

		template <typename TArgument, typename TProgress>
		struct AsynOperationHelper<ABI::Windows::Foundation::IAsyncOperationWithProgress<TArgument, TProgress>> final
		{
			using TOperation = ABI::Windows::Foundation::IAsyncOperationWithProgress<TArgument, TProgress>;
			using TResult = typename AsyncOperationTaskHelper<typename ABI::Windows::Foundation::Internal::GetAbiType<typename TOperation::TResult_complex>::type>::TResult;
			using THandler = ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler<TArgument, TProgress>;
		};

		template <typename TArgument, typename TProgress>
		struct AsynOperationHelper<RemoveIUnknown<ABI::Windows::Foundation::IAsyncOperationWithProgress<TArgument, TProgress>>> final
		{
			using TOperation = ABI::Windows::Foundation::IAsyncOperationWithProgress<TArgument, TProgress>;
			using TResult = typename AsyncOperationTaskHelper<typename ABI::Windows::Foundation::Internal::GetAbiType<typename TOperation::TResult_complex>::type>::TResult;
			using THandler = ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler<TArgument, TProgress>;
		};

#pragma endregion
	}

#pragma region ComException

	class ComException final : public std::exception
	{
	public:
		explicit ComException(HRESULT hr) NOEXCEPT
			: exception(GetString(hr).data())
			  ,_hr(hr)
		{
#ifdef TEST
			OutputDebugStringA(std::string(exception::what()).append("\r\n").data());
#endif
		}

		ComException(const ComException& other) NOEXCEPT
			: _hr(other._hr) {}

		ComException& operator=(const ComException& other) NOEXCEPT
		{
			if (this != &other)
			{
				_hr = other._hr;
			}

			return *this;
		}

		HRESULT GetResult() const NOEXCEPT
		{
			return _hr;
		}

	private:
		HRESULT _hr;

		static std::string GetString(HRESULT hr) NOEXCEPT
		{
			using namespace std;

			stringstream stream;
			stream << "ComException: " << hex << hr;
			return stream.str();
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

	class HString final : public Internals::Handle<HStringTraits>
	{
	public:
		explicit HString(Pointer pointer = HStringTraits::Invalid())
			: Handle<HStringTraits>(pointer) { }

		HString(const wchar_t* const string,
				unsigned const length)
		{
			Check(WindowsCreateString(string,
									  length,
									  GetAddressOf()));
		}

		explicit HString(const wchar_t* const string)
			: HString(string, wcslen(string)) { }

		explicit HString(std::wstring string)
			: HString(string.data(), string.size()) { }

		template <unsigned Length>
		explicit HString(const wchar_t (&string)[Length])
			: HString(string, Length - 1) { }

		HString(const HString& other)
		{
			if (HStringTraits::Invalid() != other.Get())
			{
				Check(WindowsDuplicateString(other.Get(),
											 GetAddressOf()));
			}
		}

		HString(HString&& other) NOEXCEPT
			: Handle<HStringTraits>(other.Detach()) { }

		HString& operator=(const HString& other)
		{
			if (this != &other)
			{
				ReleaseInternal();
				Check(WindowsDuplicateString(other.Get(),
											 GetAddressOf()));
			}
			return *this;
		}

		HString& operator=(HString&& other) NOEXCEPT
		{
			if (this != &other)
			{
				Attach(other.Detach());
			}
			return *this;
		}

		friend HString operator+(const HString& lhs, const HString& rhs)
		{
			HString result;
			Check(WindowsConcatString(lhs.Get(), rhs.Get(), &result));
			return result;
		}

		friend bool operator==(const HString& lhs, const HString& rhs)
		{
			auto compareResult = 0;
			Check(WindowsCompareStringOrdinal(lhs.Get(), 
											  rhs.Get(), 
											  &compareResult));
			return compareResult == 0;
		}

		friend bool operator!=(const HString& lhs, const HString& rhs)
		{
			return !(lhs == rhs);
		}

		HString Substring(unsigned start) const
		{
			HString result;
			if (HStringTraits::Invalid() == Get())
			{
				Check(WindowsSubstring(Get(),
									   start,
									   result.GetAddressOf()));
			}
			return result;
		}

		const wchar_t* GetRawBuffer() const NOEXCEPT
		{
			if (HStringTraits::Invalid() == Get())
			{
				return nullptr;
			}
			return WindowsGetStringRawBuffer(Get(), nullptr);
		}

		const wchar_t* GetRawBuffer(unsigned* length) const NOEXCEPT
		{
			if (HStringTraits::Invalid() == Get())
			{
				return nullptr;
			}
			return WindowsGetStringRawBuffer(Get(), length);
		}

		unsigned Size() const NOEXCEPT
		{
			if (HStringTraits::Invalid() == Get())
			{
				return 0;
			}
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
		HStringReference(const wchar_t* const value,
						 unsigned length)
		{
			Check(WindowsCreateStringReference(value,
											   length,
											   &_stringHeader,
											   &_string));
		}

		explicit HStringReference(const wchar_t* const string)
			: HStringReference(string, wcslen(string)) { }

		template <unsigned Length>
		explicit HStringReference(const wchar_t (&string)[Length])
			: HStringReference(string, Length - 1) { }

		explicit HStringReference(const std::wstring& string)
			: HStringReference(string.data(), string.size()) {}

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
		HSTRING _string = nullptr;
		HSTRING_HEADER _stringHeader = {};
	};

#pragma endregion

#pragma region Module

	struct Module
	{
		static bool CanUnload() NOEXCEPT;

		static void Increment() NOEXCEPT;

		static void Decrement() NOEXCEPT;
	};

#pragma endregion

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

		STDMETHODIMP QueryInterface(const GUID& guid, void** result) NOEXCEPT override
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
		ComClass() NOEXCEPT
		{
			Module::Increment();
		}

		virtual ~ComClass() NOEXCEPT
		{
			Module::Decrement();
		}

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

	template <typename TDefaultInterface,
			  typename ... TInterfaces>
	class RuntimeClass abstract : public ComClass<TDefaultInterface,
												  TInterfaces...>
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

		friend class Internals::ComPtrRef<TClass>;

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

		friend bool operator==(const ComPtr& lhs, const ComPtr& rhs) NOEXCEPT
		{
			return lhs._pointer == rhs._pointer;
		}

		friend bool operator!=(const ComPtr& lhs, const ComPtr& rhs) NOEXCEPT
		{
			return !(lhs == rhs);
		}

		Internals::RemoveIUnknown<TClass>* operator->() const NOEXCEPT
		{
			return Get();
		}

		Internals::ComPtrRef<TClass> operator&() NOEXCEPT
		{
			return Internals::ComPtrRef<TClass>(&_pointer);
		}

		Internals::RemoveIUnknown<TClass>* Get() const NOEXCEPT
		{
			using namespace Internals;

			return static_cast<RemoveIUnknown<TClass>*>(_pointer);
		}

		TClass** GetAddressOf() NOEXCEPT
		{
			return &_pointer;
		}

		void Release() NOEXCEPT
		{
			InternalRelease();
		}

		TClass** ReleaseAndGetAddressOf() NOEXCEPT
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
			swap(_pointer, other._pointer);
		}

		friend void swap(ComPtr& lhs, ComPtr& rhs) NOEXCEPT
		{
			using std::swap;
			swap(lhs._pointer, rhs._pointer);
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

#pragma region IteratorAdapter

	template <class TIterator>
	class IteratorAdapter final : public RuntimeClass<ABI::Windows::Foundation::Collections::IIterator<typename TIterator::value_type>>
	{
		using IteratorType = ABI::Windows::Foundation::Collections::IIterator<typename TIterator::value_type>;

	public:

		IteratorAdapter(TIterator&& begin, TIterator&& end) NOEXCEPT
			: _begin(std::forward<TIterator>(begin))
			  , _end(std::forward<TIterator>(end)) { }

		STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override
		{
			*className = HString(IteratorType::z_get_rc_name_impl()).Detach();
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
		using IterableType = ABI::Windows::Foundation::Collections::IIterable<typename TCollection::value_type>;

	public:

		explicit IterableAdapter(TCollection&& collection) NOEXCEPT
			: _collection(std::forward<TCollection>(collection)) { }

		STDMETHODIMP GetRuntimeClassName(HSTRING* className) NOEXCEPT override
		{
			*className = HString(IterableType::z_get_rc_name_impl()).Detach();
			return S_OK;
		}

		STDMETHODIMP First(ABI::Windows::Foundation::Collections::IIterator<typename TCollection::value_type>** first) override
		{
			*first = new(std::nothrow) IteratorAdapter<typename TCollection::iterator>(std::begin(_collection), std::end(_collection));
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

#pragma region Delegate

	template <typename TDelegateInterface,
			  typename TCallback,
			  typename ... TArgs>
	class Delegate final : public ComClass<TDelegateInterface>
	{
	public:
		explicit Delegate(TCallback&& callback) NOEXCEPT
			: _callback(std::forward<TCallback>(callback)) { }

		Delegate(const Delegate& other) NOEXCEPT
			: _callback(other._callback) { }

		Delegate(Delegate&& other) NOEXCEPT
			: _callback(std::move(other._callback)) { }

		Delegate& operator=(const Delegate& other) NOEXCEPT
		{
			if (this != &other)
			{
				_callback = other._callback;
			}
			return *this;
		}

		Delegate& operator=(Delegate&& other) NOEXCEPT
		{
			if (this != &other)
			{
				_callback = std::move(other._callback);
			}
			return *this;
		}

		STDMETHODIMP Invoke(TArgs ... args) NOEXCEPT override
		{
			return _callback(args...);
		}

	private:
		TCallback _callback;
	};

#pragma endregion

#pragma region Iterator

	template <typename TItem>
	class Iterator final
	{
	public:
		Iterator() NOEXCEPT { }

		explicit Iterator(ABI::Windows::Foundation::Collections::IIterable<TItem>* iterable)
		{
			boolean hasCurrent;
			Check(iterable->First(&_iterator));
			Check(_iterator->get_HasCurrent(&hasCurrent));
			if (!hasCurrent) _iterator.Release();
		}

		Iterator(const Iterator& other) NOEXCEPT
			: _iterator(other._iterator) {}

		Iterator(Iterator&& other) NOEXCEPT
			: _iterator(std::move(other._iterator)) {}

		Iterator& operator=(const Iterator& other) NOEXCEPT
		{
			if (this != &other)
			{
				_iterator = other._iterator;
			}
			return *this;
		}

		Iterator& operator=(Iterator&& other) NOEXCEPT
		{
			if (this != &other)
			{
				_iterator = std::move(other._iterator);
			}
			return *this;
		}

		const TItem& operator*()
		{
			TItem item;
			Check(_iterator->get_Current(&item));
			return item;
		}

		Iterator& operator++()
		{
			boolean hasNext;
			Check(_iterator->MoveNext(&hasNext));
			if (!hasNext) _iterator.Release();
			return *this;
		}

		friend bool operator==(const Iterator& lhs, const Iterator& rhs)
		{
			return lhs._iterator == rhs._iterator;
		}

		friend bool operator!=(const Iterator& lhs, const Iterator& rhs)
		{
			return !(lhs == rhs);
		}

		friend void swap(Iterator& lhs, Iterator& rhs) NOEXCEPT
		{
			using std::swap;
			swap(lhs._iterator, rhs._iterator);
		}

	private:
		ComPtr<ABI::Windows::Foundation::Collections::IIterator<TItem>> _iterator;
	};

#pragma endregion

	template <typename TDelegateInterface,
			  typename TCallback,
			  typename ... TArgs>
	inline Delegate<TDelegateInterface, TCallback, TArgs...> ÑreateDelegate(TCallback&& callback,
																			std::tuple<TArgs...>) NOEXCEPT
	{
		return Delegate<TDelegateInterface, TCallback, TArgs...>(std::forward<TCallback>(callback));
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

		using TTuple = typename FunctionTraits<decltype(&TDelegateInterface::Invoke)>::TypesTuple;

		auto delegate = ÑreateDelegate<TDelegateInterface>(std::forward<TCallback>(callback),
														   TTuple());

		using TDelegateType = decltype(delegate);

		return ComPtr<TDelegateInterface>(new TDelegateType(std::move(delegate)));
	};

	template <typename TAsyncOperation>
	inline auto GetTask(TAsyncOperation* asyncOperation) ->
	Concurrency::task<typename Internals::AsynOperationHelper<TAsyncOperation>::TResult>
	{
		using namespace Internals;
		using namespace Concurrency;
		using namespace ABI::Windows::Foundation;

		using TAsyncOperationHelper = AsynOperationHelper<TAsyncOperation>;
		using TOperation = typename TAsyncOperationHelper::TOperation;
		using TResult = typename TAsyncOperationHelper::TResult;

		task_completion_event<TResult> taskCompletitionEvent;

		auto asyncOperationPointer = CreateComPtr(static_cast<TOperation*>(asyncOperation));

		auto callback = CreateCallback<typename TAsyncOperationHelper::THandler>(
			[asyncOperationPointer, taskCompletitionEvent]
			(TOperation* operation, AsyncStatus status) mutable ->
			HRESULT
			{
				try
				{
					ComPtr<IAsyncInfo> asyncInfo;
					Check(asyncOperationPointer.As(&asyncInfo));

					switch (status)
					{
						case AsyncStatus::Completed:
							{
								TResult result;
								Check(operation->GetResults(&result));

								taskCompletitionEvent.set(std::move(result));
								break;
							}
						case AsyncStatus::Canceled:
							{
								taskCompletitionEvent.set_exception(task_canceled());
								break;
							}
						case AsyncStatus::Error:
							{
								HRESULT errorCode;
								Check(asyncInfo->get_ErrorCode(&errorCode));

								taskCompletitionEvent.set_exception(ComException(errorCode));
								break;
							}
						case AsyncStatus::Started:
							break;
						default:
							break;
					}

					asyncOperationPointer.Release();
					Check(asyncInfo->Close());
				}
				catch (const ComException& exception)
				{
					taskCompletitionEvent.set_exception(exception);
				}

				return S_OK;
			});

		Check(asyncOperation->put_Completed(callback.Get()));

		return task<TResult>(taskCompletitionEvent);
	}

	inline void Check(HRESULT hr)
	{
#ifdef TEST
		if (IS_ERROR(hr))
		{
			throw ComException(hr);
		}
#endif
	}
}

template <typename TItem>
inline MTL::Iterator<TItem> begin(ABI::Windows::Foundation::Collections::IIterable<TItem>* iterable) NOEXCEPT
{
	using namespace MTL;
	return Iterator<TItem>(iterable);
}

template <typename TItem>
inline MTL::Iterator<TItem> end(ABI::Windows::Foundation::Collections::IIterable<TItem>*) NOEXCEPT
{
	using namespace MTL;
	return Iterator<TItem>();
}

template <typename TKey, typename TValue>
inline MTL::Iterator<ABI::Windows::Foundation::Collections::IKeyValuePair<TKey, TValue>*> begin(ABI::Windows::Foundation::Collections::IMap<TKey, TValue>* map) NOEXCEPT
{
	using namespace MTL;
	using namespace ABI::Windows::Foundation::Collections;
	using namespace ABI::Windows::Foundation;

	ComPtr<IIterable<IKeyValuePair<TKey, TValue>*>> iterable;
	map->QueryInterface<IIterable<IKeyValuePair<TKey, TValue>*>>(&iterable);

	return Iterator<IKeyValuePair<TKey, TValue>*>(iterable.Get());
}

template <typename TKey, typename TValue>
inline MTL::Iterator<ABI::Windows::Foundation::Collections::IKeyValuePair<TKey, TValue>*> end(ABI::Windows::Foundation::Collections::IMap<TKey, TValue>*) NOEXCEPT
{
	using namespace MTL;
	using namespace ABI::Windows::Foundation::Collections;

	return Iterator<IKeyValuePair<TKey, TValue>*>();
}

template <typename TKey, typename TValue>
inline MTL::Iterator<ABI::Windows::Foundation::Collections::IKeyValuePair<TKey, TValue>*> begin(ABI::Windows::Foundation::Collections::IMapView<TKey, TValue>* mapView) NOEXCEPT
{
	using namespace MTL;
	using namespace ABI::Windows::Foundation::Collections;
	using namespace ABI::Windows::Foundation;

	ComPtr<IIterable<IKeyValuePair<TKey, TValue>*>> iterable;
	mapView->QueryInterface<IIterable<IKeyValuePair<TKey, TValue>*>>(&iterable);

	return Iterator<IKeyValuePair<TKey, TValue>*>(iterable.Get());
}

template <typename TKey, typename TValue>
inline MTL::Iterator<ABI::Windows::Foundation::Collections::IKeyValuePair<TKey, TValue>*> end(ABI::Windows::Foundation::Collections::IMapView<TKey, TValue>*) NOEXCEPT
{
	using namespace MTL;
	using namespace ABI::Windows::Foundation::Collections;

	return Iterator<IKeyValuePair<TKey, TValue>*>();
}

template <typename TValue>
inline MTL::Iterator<TValue> begin(ABI::Windows::Foundation::Collections::IVector<TValue>* vector) NOEXCEPT
{
	using namespace MTL;
	using namespace ABI::Windows::Foundation::Collections;
	using namespace ABI::Windows::Foundation;

	ComPtr<IIterable<TValue>> iterable;
	vector->QueryInterface<IIterable<TValue>>(&iterable);

	return Iterator<TValue>(iterable.Get());
}

template <typename TValue>
inline MTL::Iterator<TValue> end(ABI::Windows::Foundation::Collections::IVector<TValue>*) NOEXCEPT
{
	using namespace MTL;

	return Iterator<TValue>();
}

template <typename TValue>
inline MTL::Iterator<TValue> begin(ABI::Windows::Foundation::Collections::IVectorView<TValue>* vectorView) NOEXCEPT
{
	using namespace MTL;
	using namespace ABI::Windows::Foundation::Collections;
	using namespace ABI::Windows::Foundation;

	ComPtr<IIterable<TValue>> iterable;
	vectorView->QueryInterface<IIterable<TValue>>(&iterable);

	return Iterator<TValue>(iterable.Get());
}

template <typename TValue>
inline MTL::Iterator<TValue> end(ABI::Windows::Foundation::Collections::IVectorView<TValue>*) NOEXCEPT
{
	using namespace MTL;

	return Iterator<TValue>();
}
