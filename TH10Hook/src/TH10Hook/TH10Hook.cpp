#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <boost/bind.hpp>
#include <boost/log/utility/setup/file.hpp>

#include "TH10Hook/DllMain.h"
#include "TH10Hook/D3D9Hook.h"

th::TH10Hook g_th10Hook;

#pragma data_seg("Shared")
HHOOK g_hook = nullptr;
#pragma data_seg()
#pragma comment(linker, "/SECTION:Shared,RWS")

// 只收到SendMessage的消息，收不到PostMessage的消息，需要窗口子类化
LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code < 0)
		return CallNextHookEx(g_hook, code, wParam, lParam);

	g_th10Hook.hookProc(code, wParam, lParam);

	return CallNextHookEx(g_hook, code, wParam, lParam);
}

bool WINAPI Hook(DWORD threadId)
{
	try
	{
		if (g_hook != nullptr)
			BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("已挂钩。"));

		g_hook = SetWindowsHookEx(WH_CALLWNDPROC, &HookProc, g_dllModule, threadId);
		if (g_hook == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		return true;
	}
	catch (...)
	{
		std::string info = boost::current_exception_diagnostic_information();
		BOOST_LOG_TRIVIAL(error) << info;
		return false;
	}
}

void WINAPI Unhook()
{
	if (g_hook != nullptr)
	{
		UnhookWindowsHookEx(g_hook);
		g_hook = nullptr;
	}
}

namespace th
{
	namespace blog = boost::log;

#define WM_START_HOOK (WM_USER + 0x0278)
#define WM_STOP_HOOK (WM_USER + 0x0279)

	TH10Hook::TH10Hook() :
		m_quit(false)
	{
		std::string logName = win::Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
		blog::add_file_log(logName);

		//DWORD threadId = GetCurrentThreadId();
		//HANDLE dllMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
	}

	TH10Hook::~TH10Hook()
	{
	}

	void TH10Hook::hookProc(int code, WPARAM wParam, LPARAM lParam)
	{
		LPCWPSTRUCT cwp = reinterpret_cast<LPCWPSTRUCT>(lParam);
		switch (cwp->message)
		{
		case WM_START_HOOK:
			MessageBox(nullptr, _T("startHook"), _T("TH10Hook"), MB_OK);
			break;

		case WM_STOP_HOOK:
			MessageBox(nullptr, _T("stopHook"), _T("TH10Hook"), MB_OK);
			break;

		case WM_DESTROY:
			//MessageBox(nullptr, _T("WM_DESTROY"), _T("TH10Hook"), MB_OK);
			break;
		}
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
		//WaitForSingleObject(dllMainThread, INFINITE);
		//CloseHandle(dllMainThread);

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
