#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <boost/bind.hpp>
#include <boost/log/utility/setup/file.hpp>

#include "TH10Hook/DllMain.h"
#include "TH10Hook/D3D9Hook.h"

#pragma data_seg("Shared")
HHOOK g_hook = nullptr;
HWND g_window = nullptr;
DWORD g_threadId = 0;
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
		Singleton(this),
		m_quit(false)
	{
		std::string logName = win::Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
		blog::add_file_log(logName);
	}

	TH10Hook::~TH10Hook()
	{
	}

	// 在注入进程运行
	bool TH10Hook::hook(HWND window)
	{
		try
		{
			if (g_hook != nullptr)
				BOOST_THROW_EXCEPTION(Exception() << err_str("共享钩子句柄不为空。"));

			DWORD threadId = GetWindowThreadProcessId(window, nullptr);

			g_hook = SetWindowsHookEx(WH_CALLWNDPROC, &TH10Hook::HookProc, g_dllModule, threadId);
			if (g_hook == nullptr)
				THROW_SYSTEM_EXCEPTION(GetLastError());

			g_window = window;
			g_threadId = threadId;

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
			g_window = nullptr;
			g_threadId = 0;
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
			MessageBox(nullptr, _T("WM_HOOK_D3D"), _T("TH10Hook"), MB_OK);
			break;

		case WM_UNHOOK_D3D:
		//case WM_DESTROY:
			MessageBox(nullptr, _T("WM_UNHOOK_D3D"), _T("TH10Hook"), MB_OK);
			break;
		}

		return CallNextHookEx(g_hook, code, wParam, lParam);
	}

	void TH10Hook::startHook()
	{
		m_thread = boost::thread(boost::bind(&TH10Hook::hookProc, this,
			boost::placeholders::_1), nullptr);
	}

	void TH10Hook::stopHook()
	{
		{
			boost::lock_guard<boost::mutex> lock(m_mutex);
			m_quit = true;
		}
		m_cv.notify_one();

		if (m_thread.joinable())
			m_thread.join();
	}

	void TH10Hook::hookProc(HANDLE dllMainThread)
	{
		try
		{
			D3D9Hook d3d9Hook;
			d3d9Hook.hook();

			{
				boost::unique_lock<boost::mutex> lock(m_mutex);
				while (!m_quit)
					m_cv.wait(lock);
			}

			d3d9Hook.unhook();
		}
		catch (...)
		{
			std::string info = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << info;
		}
	}
}
