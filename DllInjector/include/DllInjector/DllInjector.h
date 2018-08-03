#pragma once

#include <Windows/Process.h>

namespace di
{
	class DllInjector
	{
	public:
		static void EnableDebugPrivilege();
		static void Inject(Process& process, const std::string& dllPath);
		static void Uninject(Process& process, const std::string& dllName);
	};
}
