#include "GraphHook/Common.h"
#include "GraphHook/DllMain.h"

#include "GraphHook/GraphHook.h"

HMODULE g_dllModule = nullptr;
gh::GraphHook g_graphHook;

BOOL APIENTRY DllMain(HMODULE module, DWORD reasonForCall, LPVOID reserved)
{
	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		g_dllModule = module;
		g_graphHook.subclass();
		break;

	case DLL_PROCESS_DETACH:
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
