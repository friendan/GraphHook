#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <boost/log/utility/setup/file.hpp>

#include "TH10Hook/DllMain.h"

#pragma data_seg("Shared")
HHOOK g_hook = nullptr;
//HWND g_window = nullptr;
//DWORD g_threadId = 0;
#pragma data_seg()
#pragma comment(linker, "/SECTION:Shared,RWS")

#define WM_HOOK_D3D (WM_USER + 0x0178)
#define WM_UNHOOK_D3D (WM_USER + 0x0179)

th::TH10Hook g_th10Hook;

bool WINAPI Hook(HWND window)
{
	th::TH10Hook& th10Hook = th::TH10Hook::GetInstance();
	return th10Hook.hook(window);
}

void WINAPI Unhook()
{
	th::TH10Hook& th10Hook = th::TH10Hook::GetInstance();
	th10Hook.unhook();
}

namespace th
{
	namespace blog = boost::log;

	TH10Hook::TH10Hook() :
		Singleton(this)
	{
	}

	TH10Hook::~TH10Hook()
	{
	}

	// 在注入进程运行
	bool TH10Hook::hook(HWND window)
	{
		std::string logName = Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
		blog::add_file_log(logName);

		try
		{
			if (g_hook != nullptr)
				BOOST_THROW_EXCEPTION(Exception() << err_str("共享钩子句柄不为空。"));

			DWORD threadId = GetWindowThreadProcessId(window, nullptr);

			g_hook = SetWindowsHookEx(WH_CALLWNDPROC, &TH10Hook::HookProc, g_dllModule, threadId);
			if (g_hook == nullptr)
				THROW_SYSTEM_EXCEPTION(GetLastError());

			//g_window = window;
			//g_threadId = threadId;

			return true;
		}
		catch (...)
		{
			std::string info = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << info;
			return false;
		}
	}

	// 在注入进程运行
	void TH10Hook::unhook()
	{
		if (g_hook != nullptr)
		{
			UnhookWindowsHookEx(g_hook);
			g_hook = nullptr;
			//g_window = nullptr;
			//g_threadId = 0;
		}
	}

	// 在目标窗口线程运行
	LRESULT CALLBACK TH10Hook::HookProc(int code, WPARAM wParam, LPARAM lParam)
	{
		TH10Hook& th10Hook = TH10Hook::GetInstance();
		return th10Hook.hookProc(code, wParam, lParam);
	}

	// 只能收SendMessage消息，不能收PostMessage消息
	LRESULT TH10Hook::hookProc(int code, WPARAM wParam, LPARAM lParam)
	{
		if (code < 0)
			return CallNextHookEx(g_hook, code, wParam, lParam);

		LPCWPSTRUCT cwp = reinterpret_cast<LPCWPSTRUCT>(lParam);
		switch (cwp->message)
		{
		case WM_HOOK_D3D:
			hookD3D();
			break;

		case WM_UNHOOK_D3D:
			unhookD3D();
			break;

		case WM_DESTROY:
			break;
		}

		return CallNextHookEx(g_hook, code, wParam, lParam);
	}

	void TH10Hook::hookD3D()
	{
		std::string logName = Utils::GetModuleDir(g_dllModule) + "\\TH10Hook1.log";
		blog::add_file_log(logName);

		try
		{
			if (m_d3d9Hook == nullptr)
			{
				m_d3d9Hook = std::make_shared<D3D9Hook>();
				m_d3d9Hook->hook();
			}
		}
		catch (...)
		{
			std::string info = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << info;
		}
	}

	void TH10Hook::unhookD3D()
	{
		try
		{
			if (m_d3d9Hook != nullptr)
			{
				m_d3d9Hook->unhook();
				m_d3d9Hook = nullptr;
			}
		}
		catch (...)
		{
			std::string info = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << info;
		}
	}
}
