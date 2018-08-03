#include "DllInjector/Common.h"

#include <vector>
#include <boost/log/utility/setup/file.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <Windows/Process.h>
#include <Windows/Utils.h>

#include "DllInjector/DllInjector.h"

namespace blog = boost::log;
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

int APIENTRY _tWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPTSTR cmdLine, int cmdShow)
{
	try
	{
		std::string logName = win::Utils::GetModuleDir() + "\\DllInjector.log";
		blog::add_file_log(logName);

		bpo::options_description desc("Allowed options");
		desc.add_options()
			("inject", "inject dll")
			("uninject", "uninject dll")
			("pid", bpo::value<DWORD>(), "process id")
			("dll", bpo::value<std::string>(), "dll name");

		std::vector<std::wstring> args = bpo::split_winmain(cmdLine);
		bpo::variables_map vm;
		bpo::store(bpo::wcommand_line_parser(args).options(desc).run(), vm);

		if (vm.count("inject") && !vm.count("uninject"))
		{
			DWORD pid = vm["pid"].as<DWORD>();
			std::string dllName = vm["dll"].as<std::string>();

			std::string dllPath = win::Utils::GetModuleDir() + "\\" + dllName;
			if (!bfs::exists(dllPath))
				BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("文件不存在：" + dllPath));

			win::Process process(pid);
			di::DllInjector::EnableDebugPrivilege();
			di::DllInjector::Inject(process, dllPath);
		}
		else if (vm.count("uninject") && !vm.count("inject"))
		{
			DWORD pid = vm["pid"].as<DWORD>();
			std::string dllName = vm["dll"].as<std::string>();

			win::Process process(pid);
			di::DllInjector::EnableDebugPrivilege();
			di::DllInjector::Uninject(process, dllName);
		}
		else
		{
			BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("错误的参数。"));
		}

		return 0;
	}
	catch (...)
	{
		std::string info = boost::current_exception_diagnostic_information();
		BOOST_LOG_TRIVIAL(error) << info;
		//MessageBox(nullptr, _T("发生异常，详细信息请查看DllInjector.log。"), _T("DllInjector"), MB_OK | MB_ICONERROR);
		return 1;
	}
}
