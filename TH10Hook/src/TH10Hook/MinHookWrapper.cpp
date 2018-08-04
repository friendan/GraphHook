#include "TH10Hook/Common.h"
#include "TH10Hook/MinHookWrapper.h"

#include <MinHook.h>

namespace th
{
	MinHookWrapper::MinHookWrapper()
	{
		MH_STATUS status = MH_Initialize();
		if (status != MH_OK)
			BOOST_THROW_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));
	}

	MinHookWrapper::~MinHookWrapper()
	{
		MH_Uninitialize();
	}

	void MinHookWrapper::hookAll()
	{
		MH_STATUS status = MH_EnableHook(MH_ALL_HOOKS);
		if (status != MH_OK)
			BOOST_THROW_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));
	}

	void MinHookWrapper::unhookAll()
	{
		MH_STATUS status = MH_DisableHook(MH_ALL_HOOKS);
		if (status != MH_OK)
			BOOST_THROW_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));
	}
}
