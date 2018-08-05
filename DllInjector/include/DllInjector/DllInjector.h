#pragma once

#include <Windows/Process.h>

namespace di
{
	class DllInjector
	{
	public:
		static void Inject(Process& target, const std::string& dllPath);
		static void Uninject(Process& target, const std::string& dllName);

		static void HookProc(const std::string& dllPath, const std::string& hookFuncName,
			const std::string& unhookFuncName, DWORD threadId, const std::string& hookMutexName,
			const std::string& hookCondName);
	};
}
