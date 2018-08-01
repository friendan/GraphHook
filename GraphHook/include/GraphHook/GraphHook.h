#pragma once

#include <boost/thread.hpp>

namespace gh
{
	class GraphHook
	{
	public:
		static void StartHook();
		static void StopHook();

		GraphHook();
		~GraphHook();

	private:
		static void HookProc(HANDLE dllMainThread);

		static boost::thread s_hookThread;

	};
}
