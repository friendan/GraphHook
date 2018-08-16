#include "DllInjector/Common.h"

#include <vector>
#include <sstream>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/locale.hpp>
#include <Windows/Process.h>
#include <Windows/Window.h>

#include "DllInjector/DllInjector.h"

namespace bpo = boost::program_options;
namespace blog = boost::log;
namespace blc = boost::locale::conv;

int APIENTRY _tWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPTSTR cmdLine, int cmdShow)
{
	std::string logName = win::Utils::GetModuleDir() + "\\DllInjector.log";
	blog::add_file_log(logName);

	try
	{
		bpo::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			("remote-thread", u8"远程线程注入。")
			("process-id", bpo::value<DWORD>(), u8"目标进程ID。")
			("process-name", bpo::value<std::string>(), u8"目标进程名。")
			("inject-dll", bpo::value<std::string>(), u8"注入DLL名。")
			("uninject-dll", bpo::value<std::string>(), u8"卸载DLL名。")
			("windows-hook", u8"消息钩子注入。")
			("thread-id", bpo::value<DWORD>(), u8"目标线程ID。")
			("window-name", bpo::value<std::string>(), u8"目标窗口名。")
			("class-name", bpo::value<std::string>(), u8"目标窗口类名。")
			("hook-dll", bpo::value<std::string>(), u8"钩子DLL名。")
			("hook-func", bpo::value<std::string>()->default_value("Hook"), u8"挂钩函数名。")
			("unhook-func", bpo::value<std::string>()->default_value("Unhook"), u8"脱钩函数名。")
			("hook-event", bpo::value<std::string>()->default_value("HookEvent"), u8"挂钩事件名。")
			("unhook-event", bpo::value<std::string>()->default_value("UnhookEvent"), u8"脱钩事件名。");

		std::vector<std::wstring> args = bpo::split_winmain(cmdLine);
		bpo::variables_map vm;
		bpo::store(bpo::wcommand_line_parser(args).options(desc).run(), vm);

		bool invalidArgs = false;
		if (vm.count("help"))
		{
			std::ostringstream oss;
			oss << desc;
			std::wstring descW = blc::utf_to_utf<wchar_t>(oss.str());
			MessageBox(nullptr, descW.c_str(), _T("DllInjector"), MB_OK | MB_ICONINFORMATION);
		}
		else if (vm.count("remote-thread"))
		{
			win::Process target;
			if (vm.count("process-id"))
			{
				DWORD processId = vm["process-id"].as<DWORD>();
				target = win::Process(processId);
			}
			else if (vm.count("process-name"))
			{
				std::string processName = vm["process-name"].as<std::string>();
				target = win::Process::FindByName(processName);
			}
			else
			{
				invalidArgs = true;
			}
			if (!invalidArgs)
			{
				if (vm.count("inject-dll"))
				{
					std::string dllName = vm["inject-dll"].as<std::string>();
					win::Process::EnableDebugPrivilege();
					di::DllInjector::Inject(target, dllName);
				}
				else if (vm.count("uninject-dll"))
				{
					std::string dllName = vm["uninject-dll"].as<std::string>();
					win::Process::EnableDebugPrivilege();
					di::DllInjector::Uninject(target, dllName);
				}
				else
				{
					invalidArgs = true;
				}
			}
		}
		else if (vm.count("windows-hook"))
		{
			DWORD threadId = 0;
			if (vm.count("thread-id"))
			{
				threadId = vm["thread-id"].as<DWORD>();
			}
			else if (vm.count("window-name"))
			{
				std::string windowName = vm["window-name"].as<std::string>();
				win::Window window = win::Window::FindByName(windowName);
				threadId = window.getThreadId();
			}
			else if (vm.count("class-name"))
			{
				std::string className = vm["class-name"].as<std::string>();
				win::Window window = win::Window::FindByClassName(className);
				threadId = window.getThreadId();
			}
			else
			{
				invalidArgs = true;
			}
			if (!invalidArgs)
			{
				std::string dllName = vm["hook-dll"].as<std::string>();
				std::string hookFuncName = vm["hook-func"].as<std::string>();
				std::string unhookFuncName = vm["unhook-func"].as<std::string>();
				std::string hookEventName = vm["hook-event"].as<std::string>();
				std::string unhookEventName = vm["unhook-event"].as<std::string>();
				win::Process::EnableDebugPrivilege();
				di::DllInjector::HookProc(threadId, dllName, hookFuncName, unhookFuncName,
					hookEventName, unhookEventName);
			}
		}
		else
		{
			invalidArgs = true;
		}

		if (invalidArgs)
			THROW_CPP_EXCEPTION(cpp::Exception() << cpp::err_str(u8"错误的参数。"));

		return 0;
	}
	catch (...)
	{
		std::string info = boost::current_exception_diagnostic_information();
		BOOST_LOG_TRIVIAL(error) << info;
		return 1;
	}
}
