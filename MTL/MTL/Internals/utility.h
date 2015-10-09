#pragma once
#include <Windows.h>
#include <array>
#include <tuple>
#include <type_traits>
#include <macro.h>

namespace MTL
{
	namespace Internals
	{
		template <typename Interface>
		struct Cloaked : Interface {};

		template <typename ... Ts>
		struct types_pack
		{
		};

#pragma region is_cloaked

		template <typename Interface>
		struct is_cloaked : std::false_type
		{
		};

		template <typename Interface>
		struct is_cloaked <Cloaked<Interface>> : std::true_type
		{
		};

#pragma endregion

#pragma region variadic_is_base_of

		template <typename TBase, typename T, typename ... Ts>
		struct variadic_is_base_of : std::conditional<std::is_base_of<TBase, T>::value, variadic_is_base_of<TBase, Ts...>, std::false_type>::type
		{
		};

		template <typename TBase, typename T>
		struct variadic_is_base_of<TBase, T> : std::is_base_of<TBase, T>
		{
		};

#pragma endregion

#pragma region variadic_is_same
		
		template <typename TLhs, typename TRhs, typename ... Ts>
		struct variadic_is_same : std::conditional<std::is_same<TLhs, TRhs>::value, std::true_type, variadic_is_same<TLhs, Ts...>>::type
		{
		};

		template <typename TLhs, typename TRhs>
		struct variadic_is_same<TLhs, TRhs> : std::is_same<TLhs, TRhs>
		{
		};

#pragma endregion

#pragma region is_type_set
		
		template <typename T, typename ... Ts>
		struct is_type_set : std::conditional<variadic_is_same<T, Ts...>::value, std::false_type, is_type_set<Ts...>>::type
		{
		};

		template <typename T>
		struct is_type_set<T> : std::true_type
		{
		};

#pragma endregion

#pragma region interface_counter

		template <typename T, typename ... Ts>
		struct interface_counter
		{
			static CONSTEXPR unsigned typesCount = interface_counter<Ts...>::typesCount + static_cast<unsigned>(!is_cloaked<T>::value);
		};

		template <typename T>
		struct interface_counter<T>
		{
			static CONSTEXPR unsigned typesCount = static_cast<unsigned>(!is_cloaked<T>::value);
		};

#pragma endregion

		template <typename ... Ts>
		struct iids_holder
		{
			static GUID* getIids() NOEXCEPT
			{
				return (new std::array<GUID, sizeof...(Ts)>{ __uuidof(Ts)... })->data();
			}
		};

		template <unsigned Counter, typename ... Ts>
		struct cloaked_filter;

		template <unsigned Counter>
		struct cloaked_filter<Counter> : iids_holder<>
		{
		};

		template <unsigned Counter, typename T, typename ... Ts>
		struct cloaked_filter<Counter, T, Ts...> :
			std::conditional<	Counter == sizeof...(Ts) + 1,
								typename iids_holder<T, Ts...>, 
								typename std::conditional<is_cloaked<T>::value, cloaked_filter<Counter, Ts...>, cloaked_filter<Counter + 1, Ts..., T>>::type>::type
		{
		};

		template <typename T, typename ... Ts>
		struct iids_extractor : cloaked_filter<0, T, Ts...>
		{

		};

#pragma region arg_traits

		template<typename TMemberFunction>
		struct arg_traits
		{
			static CONSTEXPR int argsCount = -1;
		};

		template<typename TDelegateInterface>
		struct arg_traits<HRESULT(STDMETHODCALLTYPE TDelegateInterface::*)(void)>
		{
			static CONSTEXPR int argsCount = 0;
			typedef types_pack<void> types;
		};

		template<typename TDelegateInterface, typename ... TArgs>
		struct arg_traits<HRESULT(STDMETHODCALLTYPE TDelegateInterface::*)(TArgs...)>
		{
			static CONSTEXPR int argsCount = sizeof...(TArgs);
			typedef types_pack<TArgs...> types;
		};

#pragma endregion

	}
}

