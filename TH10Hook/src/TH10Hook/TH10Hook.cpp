#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <fstream>
#include <boost/bind.hpp>
//#include <boost/log/utility/setup/file.hpp>
#include <boost/locale.hpp>
#include <Windows/Utils.h>

#include "TH10Hook/DllMain.h"
#include "TH10Hook/D3D9Hook.h"

//namespace blog = boost::log;
namespace blc = boost::locale::conv;

th::TH10Hook g_th10Hook;
HHOOK g_hook = nullptr;

std::ofstream ofs("1.txt");

LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	ofs << (intptr_t)g_hook << " " << code << std::endl;
	return CallNextHookEx(g_hook, code, wParam, lParam);
}

extern "C" __declspec(dllexport) bool Hook(DWORD threadId)
{
	std::string logName = win::Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
	//blog::add_file_log(logName);

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
		//BOOST_LOG_TRIVIAL(error) << info;
		std::wstring logNameW = blc::utf_to_utf<wchar_t>(logName);
		std::ofstream ofs(logNameW);
		ofs << info;
		return false;
	}
}

extern "C" __declspec(dllexport) void Unhook()
{
	std::string logName = win::Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
	//blog::add_file_log(logName);

	try
	{
		if (!UnhookWindowsHookEx(g_hook))
			THROW_SYSTEM_EXCEPTION(GetLastError());
	}
	catch (...)
	{
		std::string info = boost::current_exception_diagnostic_information();
		//BOOST_LOG_TRIVIAL(error) << info;
		std::wstring logNameW = blc::utf_to_utf<wchar_t>(logName);
		std::ofstream ofs(logNameW);
		ofs << info;
	}
}

namespace th
{
	TH10Hook::TH10Hook() :
		m_quit(false)
	{
		//DWORD threadId = GetCurrentThreadId();
		//HANDLE dllMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);

		//m_thread = boost::thread(boost::bind(&TH10Hook::hookProc, this,
		//	boost::placeholders::_1), nullptr);
	}

	TH10Hook::~TH10Hook()
	{
		//{
		//	boost::lock_guard<boost::mutex> lock(m_mutex);
		//	m_quit = true;
		//}
		//m_cv.notify_one();

		//if (m_thread.joinable())
		//	m_thread.join();
	}

	void TH10Hook::hookProc(HANDLE dllMainThread)
	{
		//WaitForSingleObject(dllMainThread, INFINITE);
		//CloseHandle(dllMainThread);

		std::string logName = Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
		//blog::add_file_log(logName);

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
			//BOOST_LOG_TRIVIAL(error) << info;
			std::wstring logNameW = blc::utf_to_utf<wchar_t>(logName);
			std::ofstream ofs(logNameW);
			ofs << info;
		}
	}
}
