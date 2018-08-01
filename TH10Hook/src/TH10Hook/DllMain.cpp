#include "TH10Hook/Common.h"
#include "TH10Hook/DllMain.h"

#include "TH10Hook/TH10Hook.h"

HMODULE g_dllModule = nullptr;

BOOL APIENTRY DllMain(HMODULE module, DWORD reasonForCall, LPVOID reserved)
{
	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		g_dllModule = module;
		th::TH10Hook::StartHook();
		break;

	case DLL_PROCESS_DETACH:
		th::TH10Hook::StopHook();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
