#pragma once
#include "IAuthorizer.h"

namespace EasyFree
{
	namespace Background
	{
		namespace Internals
		{
			class MosMetroAuthorizer final : public IAuthorizer
			{
			public:
				virtual Concurrency::task<bool> Authorize() const NOEXCEPT override;

				virtual bool CanAuth(ABI::Windows::Networking::Connectivity::IConnectionProfile* connectionProfile) const NOEXCEPT override;
			};
		}
	}
}
