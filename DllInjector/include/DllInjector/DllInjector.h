#pragma once

#include <Windows/Process.h>

namespace di
{
	class DllInjector
	{
	public:
		static void Inject(Process& target, const std::string& dllPath);
		static void Uninject(Process& target, const std::string& dllName);

		static void Hook(const std::string& dllPath, const std::string& hookFuncName,
			const std::string& unhookFuncName, const std::string& className,
			const std::string& windowName, const std::string& namedMutexName,
			const std::string& namedCondName);
	};
}
