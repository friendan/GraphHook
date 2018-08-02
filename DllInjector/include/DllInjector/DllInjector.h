#pragma once

#include <Windows/Process.h>

namespace di
{
	class DllInjector
	{
	public:
		void enableDebugPrivilege();
		void inject(Process& process, const std::string& dllPath);
		void uninject(Process& process, const std::string& dllName);
	};
}
