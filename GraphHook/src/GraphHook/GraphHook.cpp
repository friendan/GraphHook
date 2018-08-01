#include "GraphHook/Common.h"
#include "GraphHook/GraphHook.h"

#include <boost/bind.hpp>

namespace gh
{
	boost::thread GraphHook::s_hookThread;

	void GraphHook::StartHook()
	{
		DWORD threadId = GetCurrentThreadId();
		HANDLE dllMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);

		s_hookThread = boost::thread(boost::bind(&GraphHook::HookProc,
			boost::placeholders::_1), dllMainThread);
	}

	void GraphHook::StopHook()
	{
		s_hookThread.join();
	}

	void GraphHook::HookProc(HANDLE dllMainThread)
	{
		WaitForSingleObject(dllMainThread, INFINITE);
		CloseHandle(dllMainThread);


	}

	GraphHook::GraphHook()
	{
		//MessageBox(nullptr, _T("×¢ÈëDLL¡£"), _T("GraphHook"), MB_OK);
	}

	GraphHook::~GraphHook()
	{
		//MessageBox(nullptr, _T("Ð¶ÔØDLL¡£"), _T("GraphHook"), MB_OK);
	}
}
