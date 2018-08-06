#include "DllInjector/Common.h"

#include <vector>
#include <boost/log/utility/setup/file.hpp>
#include <boost/program_options.hpp>
#include <boost/locale.hpp>
#include <Windows/Process.h>
#include <Windows/Utils.h>

#include "DllInjector/DllInjector.h"

namespace blog = boost::log;
namespace bpo = boost::program_options;
namespace blc = boost::locale::conv;

int APIENTRY _tWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPTSTR cmdLine, int cmdShow)
{
	try
	{
		std::string logName = win::Utils::GetModuleDir() + "\\DllInjector.log";
		blog::add_file_log(logName);

		bpo::options_description desc("Allowed options");
		desc.add_options()
			("remote-thread", "Զ���߳�ע�롣")
			("inject-dll", bpo::value<std::string>(), "ע��DLL����")
			("uninject-dll", bpo::value<std::string>(), "ж��DLL����")
			("process-id", bpo::value<DWORD>(), "Ŀ�����ID��")
			("process-name", bpo::value<std::string>(), "Ŀ���������")
			("windows-hook", "��Ϣ����ע�롣")
			("hook-dll", bpo::value<std::string>(), "����DLL����")
			("hook-func", bpo::value<std::string>(), "�ҹ���������")
			("unhook-func", bpo::value<std::string>(), "�ѹ���������")
			("thread-id", bpo::value<DWORD>(), "Ŀ�괰���߳�ID��")
			("class-name", bpo::value<std::string>(), "Ŀ�괰��������")
			("window-name", bpo::value<std::string>(), "Ŀ�괰������")
			("hook-mutex", bpo::value<std::string>(), "���ӻ����������")
			("hook-cond", bpo::value<std::string>(), "����������������");

		std::vector<std::wstring> args = bpo::split_winmain(cmdLine);
		bpo::variables_map vm;
		bpo::store(bpo::wcommand_line_parser(args).options(desc).run(), vm);

		bool invalidArgs = false;
		if (vm.count("remote-thread"))
		{
			DWORD processId = 0;
			if (vm.count("process-id"))
			{
				processId = vm["process-id"].as<DWORD>();
			}
			else if (vm.count("process-name"))
			{
				std::string processName = vm["process-name"].as<std::string>();
				processId = win::Process::FindIdByName(processName);
				if (processId == 0)
					BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("���ҽ���IDʧ�ܣ�" + processName));
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
					win::Process target(processId);
					di::DllInjector::Inject(target, dllName);
				}
				else if (vm.count("uninject-dll"))
				{
					std::string dllName = vm["uninject-dll"].as<std::string>();

					win::Process::EnableDebugPrivilege();
					win::Process target(processId);
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
			else if (vm.count("class-name"))
			{
				std::string className = vm["class-name"].as<std::string>();
				std::wstring classNameW = blc::utf_to_utf<wchar_t>(className);
				HWND window = FindWindow(classNameW.c_str(), nullptr);
				if (window == nullptr)
					BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("���Ҵ���ʧ�ܣ�" + className));
				threadId = GetWindowThreadProcessId(window, nullptr);
				if (threadId == 0)
					BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("��ȡ�����߳�IDʧ�ܣ�" + className));
			}
			else if (vm.count("window-name"))
			{
				std::string windowName = vm["window-name"].as<std::string>();
				std::wstring windowNameW = blc::utf_to_utf<wchar_t>(windowName);
				HWND window = FindWindow(nullptr, windowNameW.c_str());
				if (window == nullptr)
					BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("���Ҵ���ʧ�ܣ�" + windowName));
				threadId = GetWindowThreadProcessId(window, nullptr);
				if (threadId == 0)
					BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("��ȡ�����߳�IDʧ�ܣ�" + windowName));
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

				std::string hookMutexName = "hookMutex";
				std::string hookCondName = "hookCond";
				if (vm.count("hook-mutex"))
					hookMutexName = vm["hook-mutex"].as<std::string>();
				if (vm.count("hook-cond"))
					hookCondName = vm["hook-cond"].as<std::string>();

				win::Process::EnableDebugPrivilege();
				di::DllInjector::HookProc(dllName, hookFuncName, unhookFuncName, threadId,
					hookMutexName, hookCondName);
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
