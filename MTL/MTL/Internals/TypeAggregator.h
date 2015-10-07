#pragma once

namespace MTL
{
	namespace Internals
	{
		template <typename T, typename ... Ts>
		class NOVTABLE TypeAggregator abstract : public T, public Ts...
		{
		};
	}
}