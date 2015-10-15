#pragma once
#include <string>
#include <typeinfo>
#include <macro.h>

namespace MTL
{
	namespace Internals
	{
		template <typename TAggegator, typename ... Ts>
		struct ConsistencyChecker 
		{
			static void Check(TAggegator*) NOEXCEPT;
		};

		template <typename TAggegator, typename T, typename ... Ts>
		struct ConsistencyChecker<TAggegator, T, Ts...>
		{
			static void Check(TAggegator* aggregator) NOEXCEPT
			{
				if (nullptr == aggregator) return;

				auto pInspectable = reinterpret_cast<IInspectable*>(aggregator);
				auto pTypeStatic = static_cast<T*>(aggregator);

				T* pTypeQuery = nullptr;
				pInspectable->QueryInterface(&pTypeQuery);

				auto staticDif = reinterpret_cast<IInspectable*>(pTypeStatic) - reinterpret_cast<IInspectable*>(pInspectable);
				auto dynamicDif = reinterpret_cast<IInspectable*>(pTypeQuery) - reinterpret_cast<IInspectable*>(pInspectable);

				if (staticDif != dynamicDif)
				{
					OutputDebugStringA((std::string(typeid(T).name()) + "\r\n").data());
					OutputDebugStringA((std::string("Static dif: ") + std::to_string(staticDif) + "\r\n").data());
					OutputDebugStringA((std::string("Dynamic dif: ") + std::to_string(dynamicDif) + "\r\n").data());

					ULONG count;
					IID* piids;
					pInspectable->GetIids(&count, &piids);

					for (auto i = 0; i < count; i++)
					{
						wchar_t temp[40];
						StringFromGUID2(piids[i], &temp[0], std::extent<decltype(temp)>::value);
						OutputDebugStringW(temp);
						OutputDebugStringA("\r\n");
					}
				}
				ASSERT(staticDif == dynamicDif);

				ConsistencyChecker<TAggegator, Ts...>::Check(aggregator);
			}
		};

		template <typename TAggegator>
		struct ConsistencyChecker<TAggegator>
		{
			static void Check(TAggegator*) NOEXCEPT
			{
			}
		};

		template <typename TAggegator, typename T, typename ... Ts>
		void CheckConsistency(TAggegator* aggregator) NOEXCEPT
		{
			ConsistencyChecker<TAggegator, T, Ts...>::Check(aggregator);
		}
	}
}
