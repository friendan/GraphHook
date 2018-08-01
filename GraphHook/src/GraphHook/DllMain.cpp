#include "GraphHook/Common.h"
#include "GraphHook/DllMain.h"

#include "GraphHook/GraphHook.h"

HMODULE g_dllModule = nullptr;

BOOL APIENTRY DllMain(HMODULE module, DWORD reasonForCall, LPVOID reserved)
{
	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		g_dllModule = module;
		gh::GraphHook::StartHook();
		break;

	case DLL_PROCESS_DETACH:
		gh::GraphHook::StopHook();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
