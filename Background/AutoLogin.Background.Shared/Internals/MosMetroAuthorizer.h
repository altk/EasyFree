#pragma once
#include "IAuthorizer.h"

namespace AutoLogin
{
	namespace Background
	{
		namespace Internals
		{
			class MosMetroAuthorizer final : public IAuthorizer
			{
			public:
				virtual Concurrency::task<bool> Authorize(ABI::Windows::Networking::Connectivity::IConnectionProfile* connectionProfile) const NOEXCEPT override;
			};
		}
	}
}
