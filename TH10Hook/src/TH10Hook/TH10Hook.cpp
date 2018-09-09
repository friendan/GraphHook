#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <boost/log/utility/setup/file.hpp>
#include <Windows/Window.h>

#include "TH10Hook/DllMain.h"

#pragma data_seg("Shared")
HHOOK g_hook = nullptr;
DWORD g_threadId = 0;
HWND g_window = nullptr;
#pragma data_seg()
#pragma comment(linker, "/SECTION:Shared,RWS")

#define WM_HOOK_D3D (WM_USER + 0x0178)
#define WM_UNHOOK_D3D (WM_USER + 0x0179)

th::TH10Hook g_th10Hook;

bool WINAPI Hook(DWORD threadId)
{
	th::TH10Hook& th10Hook = th::TH10Hook::GetInstance();
	return th10Hook.hook(threadId);
}

void WINAPI Unhook()
{
	th::TH10Hook& th10Hook = th::TH10Hook::GetInstance();
	th10Hook.unhook();
}

namespace th
{
	namespace bl = boost::log;

	TH10Hook::TH10Hook() :
		Singleton(this)
	{
	}

	TH10Hook::~TH10Hook()
	{
	}

	// 在注入进程运行
	bool TH10Hook::hook(DWORD threadId)
	{
		std::string logName = Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
		bl::add_file_log(logName);

		try
		{
			if (g_hook != nullptr)
				THROW_CPP_EXCEPTION(Exception() << err_str("共享钩子句柄不为空。"));

			g_hook = SetWindowsHookEx(WH_CALLWNDPROC, &TH10Hook::HookProc, g_dllModule, threadId);
			if (g_hook == nullptr)
				THROW_WINDOWS_EXCEPTION(GetLastError());

			g_threadId = threadId;
			g_window = Window::FindByThreadId(threadId);

			return true;
		}
		catch (...)
		{
			std::string what = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << what;
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
			g_threadId = 0;
			g_window = nullptr;
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
		if (cwp != nullptr)
		{
			switch (cwp->message)
			{
			case WM_HOOK_D3D:
				hookD3D();
				break;

			case WM_UNHOOK_D3D:
				unhookD3D();
				break;

			case WM_DESTROY:
				//if (cwp->hwnd == g_window)
				//	unhookD3D();
				break;
			}
		}

		return CallNextHookEx(g_hook, code, wParam, lParam);
	}

	void TH10Hook::hookD3D()
	{
		std::string logName = Utils::GetModuleDir(g_dllModule) + "\\TH10Hook1.log";
		bl::add_file_log(logName);

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
			std::string what = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << what;
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
			std::string what = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << what;
		}
	}
}
