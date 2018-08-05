#include "TH10Hook/Common.h"
#include "TH10Hook/DllMain.h"

#include "TH10Hook/TH10Hook.h"

HMODULE g_dllModule = nullptr;
//cpp::sp<th::TH10Hook> g_th10Hook;
th::TH10Hook g_th10Hook;

BOOL APIENTRY DllMain(HMODULE module, DWORD reasonForCall, LPVOID reserved)
{
	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		g_dllModule = module;
		//g_th10Hook = cpp::MakeObject<th::TH10Hook>();
		break;

	case DLL_PROCESS_DETACH:
		//g_th10Hook = nullptr;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
