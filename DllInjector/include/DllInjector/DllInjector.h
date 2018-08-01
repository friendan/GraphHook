#pragma once

#include <Windows/Process.h>

namespace di
{
	class DllInjector
	{
	public:
		void enableDebugPrivilege();
		void injectDll(Process& process, const std::string& dllPath);
		void uninjectDll(Process& process, const std::string& dllName);
	};
}
