#include "DllInjector/Common.h"

#include <vector>
#include <boost/program_options.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <Windows/Process.h>
#include <Windows/Window.h>

#include "DllInjector/DllInjector.h"

namespace bpo = boost::program_options;
namespace blog = boost::log;

int APIENTRY _tWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPTSTR cmdLine, int cmdShow)
{
	std::string logName = win::Utils::GetModuleDir() + "\\DllInjector.log";
	blog::add_file_log(logName);

	try
	{
		bpo::options_description desc("Allowed options");
		desc.add_options()
			("remote-thread", "Զ���߳�ע�롣")
			("inject-dll", bpo::value<std::string>(), "ע��DLL����")
			("uninject-dll", bpo::value<std::string>(), "ж��DLL����")
			("process-name", bpo::value<std::string>(), "Ŀ���������")
			("windows-hook", "��Ϣ����ע�롣")
			("hook-dll", bpo::value<std::string>(), "����DLL����")
			("window-name", bpo::value<std::string>(), "Ŀ�괰������")
			("class-name", bpo::value<std::string>(), "Ŀ�괰��������");

		std::vector<std::wstring> args = bpo::split_winmain(cmdLine);
		bpo::variables_map vm;
		bpo::store(bpo::wcommand_line_parser(args).options(desc).run(), vm);

		bool invalidArgs = false;
		if (vm.count("remote-thread"))
		{
			if (vm.count("inject-dll"))
			{
				std::string dllName = vm["inject-dll"].as<std::string>();
				std::string processName = vm["process-name"].as<std::string>();
				win::Process::EnableDebugPrivilege();
				win::Process target = win::Process::FindByName(processName);
				di::DllInjector::Inject(dllName, target);
			}
			else if (vm.count("uninject-dll"))
			{
				std::string dllName = vm["uninject-dll"].as<std::string>();
				std::string processName = vm["process-name"].as<std::string>();
				win::Process::EnableDebugPrivilege();
				win::Process target = win::Process::FindByName(processName);
				di::DllInjector::Uninject(dllName, target);
			}
			else
			{
				invalidArgs = true;
			}
		}
		else if (vm.count("windows-hook"))
		{
			win::Window target;
			if (vm.count("window-name"))
			{
				std::string windowName = vm["window-name"].as<std::string>();
				target = win::Window::FindByName(windowName);
			}
			else if (vm.count("class-name"))
			{
				std::string className = vm["class-name"].as<std::string>();
				target = win::Window::FindByClassName(className);
			}
			else
			{
				invalidArgs = true;
			}
			if (!invalidArgs)
			{
				std::string dllName = vm["hook-dll"].as<std::string>();
				win::Process::EnableDebugPrivilege();
				di::DllInjector::HookProc(dllName, target);
			}
		}
		else
		{
			invalidArgs = true;
		}

		if (invalidArgs)
			BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("����Ĳ�����"));

		return 0;
	}
	catch (...)
	{
		std::string info = boost::current_exception_diagnostic_information();
		BOOST_LOG_TRIVIAL(error) << info;
		return 1;
	}
}
