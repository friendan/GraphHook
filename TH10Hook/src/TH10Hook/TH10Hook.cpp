#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <boost/bind.hpp>

namespace th
{
	TH10Hook::TH10Hook()
	{
		MessageBox(nullptr, _T("×¢ÈëDLL¡£"), _T("TH10Hook"), MB_OK);

		//DWORD threadId = GetCurrentThreadId();
		//HANDLE dllMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);

		//s_hookThread = boost::thread(boost::bind(&TH10Hook::HookProc, this,
		//	boost::placeholders::_1), dllMainThread);
	}

	TH10Hook::~TH10Hook()
	{
		//s_hookThread.join();

		MessageBox(nullptr, _T("Ð¶ÔØDLL¡£"), _T("TH10Hook"), MB_OK);
	}

	void TH10Hook::HookProc(HANDLE dllMainThread)
	{
		WaitForSingleObject(dllMainThread, INFINITE);
		CloseHandle(dllMainThread);


	}
}
