#include "TH10Hook/Common.h"
#include "TH10Hook/MinHookIniter.h"

#include <MinHook.h>

namespace th
{
	MinHookIniter::MinHookIniter()
	{
		MH_STATUS status;

		status = MH_Initialize();
		if (status != MH_OK)
			THROW_CPP_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));
	}

	MinHookIniter::~MinHookIniter()
	{
		MH_Uninitialize();
	}
}
