#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <fstream>
#include <boost/bind.hpp>
#include <boost/locale.hpp>
#include <Windows/Utils.h>

#include "TH10Hook/DllMain.h"
#include "TH10Hook/D3D9Hook.h"

namespace th
{
	namespace blc = boost::locale::conv;

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

			std::string logName = Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
			std::wstring logNameW = blc::utf_to_utf<wchar_t>(logName);
			std::ofstream ofs(logNameW);
			ofs << info;
		}
	}
}
