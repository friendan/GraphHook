#pragma once

#include <Windows/Process.h>

namespace di
{
	class DllInjector
	{
	public:
		static void Inject(Process& target, const std::string& dllPath);
		static void Uninject(Process& target, const std::string& dllName);
	};
}
