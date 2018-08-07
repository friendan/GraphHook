#pragma once

#include <Windows/Process.h>
#include <Windows/Window.h>

namespace di
{
	class DllInjector
	{
	public:
		static void Inject(const std::string& dllName, Process& target);
		static void Uninject(const std::string& dllName, Process& target);

		static void HookProc(const std::string& dllName, Window& target);
	};
}
