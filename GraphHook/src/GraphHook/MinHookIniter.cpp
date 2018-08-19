#include "GraphHook/Common.h"
#include "GraphHook/MinHookIniter.h"

#include <MinHook.h>

namespace gh
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
