#include "TH10Hook/Common.h"
#include "TH10Hook/DllMain.h"

#include <fstream>
#include <boost/locale.hpp>

#include "TH10Hook/TH10Hook.h"

namespace blc = boost::locale::conv;

HMODULE g_dllModule = nullptr;
//cpp::sp<th::TH10Hook> g_th10Hook;
th::TH10Hook g_th10Hook;

#pragma data_seg("Shared")
HHOOK g_hook = nullptr;
#pragma data_seg()
#pragma comment(linker, "/SECTION:Shared,RWS")

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

LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (g_hook == nullptr)
		MessageBox(nullptr, _T("aaaaaaaaaaa"), _T("aaaaaaaaaaa"), MB_OK);
	else
		MessageBox(nullptr, _T("bbbbbbbbbbb"), _T("bbbbbbbbbbb"), MB_OK);

	return CallNextHookEx(g_hook, code, wParam, lParam);
}

bool WINAPI Hook(DWORD threadId)
{
	try
	{
		g_hook = SetWindowsHookEx(WH_CALLWNDPROC, &HookProc, g_dllModule, threadId);
		if (g_hook == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		return true;
	}
	catch (...)
	{
		std::string info = boost::current_exception_diagnostic_information();

		std::string logName = win::Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
		std::wstring logNameW = blc::utf_to_utf<wchar_t>(logName);
		std::ofstream ofs(logNameW);
		ofs << info;
		return false;
	}
}

void WINAPI Unhook()
{
	try
	{
		if (!UnhookWindowsHookEx(g_hook))
			THROW_SYSTEM_EXCEPTION(GetLastError());

		g_hook = nullptr;
	}
	catch (...)
	{
		std::string info = boost::current_exception_diagnostic_information();

		std::string logName = win::Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
		std::wstring logNameW = blc::utf_to_utf<wchar_t>(logName);
		std::ofstream ofs(logNameW);
		ofs << info;
	}
}
