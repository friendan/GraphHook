#pragma once

#include <Windows/Process.h>

namespace di
{
	class DllInjector
	{
	public:
		static void EnableDebugPrivilege();

		static void Inject(Process& target, const std::string& dllName);
		static void Uninject(Process& target, const std::string& dllName);

		static void HookProc(DWORD threadId, const std::string& dllName,
			std::string hookFuncName, std::string unhookFuncName,
			std::string hookEventName, std::string unhookEventName);
	};
}
