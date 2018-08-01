#pragma once

#include <boost/thread.hpp>

namespace th
{
	class TH10Hook
	{
	public:
		static void StartHook();
		static void StopHook();

		TH10Hook();
		~TH10Hook();

	private:
		static void HookProc(HANDLE dllMainThread);

		static boost::thread s_hookThread;

	};
}
