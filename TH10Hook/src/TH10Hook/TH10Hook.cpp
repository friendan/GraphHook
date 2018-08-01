#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <boost/bind.hpp>

namespace th
{
	boost::thread TH10Hook::s_hookThread;

	void TH10Hook::StartHook()
	{
		DWORD threadId = GetCurrentThreadId();
		HANDLE dllMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);

		s_hookThread = boost::thread(boost::bind(&TH10Hook::HookProc,
			boost::placeholders::_1), dllMainThread);
	}

	void TH10Hook::StopHook()
	{
		s_hookThread.join();
	}

	void TH10Hook::HookProc(HANDLE dllMainThread)
	{
		WaitForSingleObject(dllMainThread, INFINITE);
		CloseHandle(dllMainThread);


	}

	TH10Hook::TH10Hook()
	{
		//MessageBox(nullptr, _T("×¢ÈëDLL¡£"), _T("GraphHook"), MB_OK);
	}

	TH10Hook::~TH10Hook()
	{
		//MessageBox(nullptr, _T("Ð¶ÔØDLL¡£"), _T("GraphHook"), MB_OK);
	}
}
