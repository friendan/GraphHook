#include "DllInjector/Common.h"
#include "DllInjector/DllInjector.h"

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/locale.hpp>
#include <cpp/ScopeGuard.h>

namespace di
{
	namespace bfs = boost::filesystem;
	namespace bi = boost::interprocess;
	namespace blc = boost::locale::conv;

	void DllInjector::Inject(Process& target, const std::string& dllName)
	{
		std::string dllPath = win::Utils::GetModuleDir() + "\\" + dllName;
		if (!bfs::exists(dllPath))
			BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("文件不存在：" + dllPath));
		std::wstring dllPathW = blc::utf_to_utf<wchar_t>(dllPath);
		size_t dllPathSize = (dllPathW.length() + 1) * sizeof(wchar_t);

		LPVOID remoteMemory = VirtualAllocEx(target.get(), nullptr, dllPathSize, MEM_RESERVE | MEM_COMMIT,
			PAGE_READWRITE);
		if (remoteMemory == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([&]()
		{
			VirtualFreeEx(target.get(), remoteMemory, 0, MEM_RELEASE);
		});

		SIZE_T written = 0;
		if (!WriteProcessMemory(target.get(), remoteMemory, dllPathW.c_str(), dllPathSize, &written))
			THROW_SYSTEM_EXCEPTION(GetLastError());
		if (written != dllPathSize)
			BOOST_THROW_EXCEPTION(Exception() << err_str("写入DLL路径的长度错误。"));

		HMODULE kernel32Dll = GetModuleHandle(_T("kernel32.dll"));
		if (kernel32Dll == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		FARPROC loadLibrary = GetProcAddress(kernel32Dll, "LoadLibraryW");
		if (loadLibrary == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		DWORD threadId = 0;
		HANDLE remoteThread = CreateRemoteThread(target.get(), nullptr, 0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary), remoteMemory, 0, &threadId);
		if (remoteThread == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([remoteThread]()
		{
			CloseHandle(remoteThread);
		});

		WaitForSingleObject(remoteThread, INFINITE);
	}

	void DllInjector::Uninject(Process& target, const std::string& dllName)
	{
		HMODULE module = target.findModuleByName(dllName);
		if (module == nullptr)
			return;

		HMODULE kernel32Dll = GetModuleHandle(_T("kernel32.dll"));
		if (kernel32Dll == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		FARPROC freeLibrary = GetProcAddress(kernel32Dll, "FreeLibrary");
		if (freeLibrary == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		DWORD threadId = 0;
		HANDLE remoteThread = CreateRemoteThread(target.get(), nullptr, 0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(freeLibrary), module, 0, &threadId);
		if (remoteThread == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([remoteThread]()
		{
			CloseHandle(remoteThread);
		});

		WaitForSingleObject(remoteThread, INFINITE);
	}

	void DllInjector::HookProc(const std::string& dllName, const std::string& hookFuncName,
		const std::string& unhookFuncName, DWORD threadId, const std::string& hookMutexName,
		const std::string& hookCondName)
	{
		std::string dllPath = win::Utils::GetModuleDir() + "\\" + dllName;
		if (!bfs::exists(dllPath))
			BOOST_THROW_EXCEPTION(cpp::Exception() << cpp::err_str("文件不存在：" + dllPath));
		std::wstring dllPathW = blc::utf_to_utf<wchar_t>(dllPath);

		HMODULE dll = LoadLibrary(dllPathW.c_str());
		if (dll == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([dll]()
		{
			FreeLibrary(dll);
		});

		typedef bool(*HookFunc_t)(DWORD);
		typedef void(*UnhookFunc_t)();

		HookFunc_t hookFunc = reinterpret_cast<HookFunc_t>(GetProcAddress(dll, hookFuncName.c_str()));
		if (hookFunc == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		UnhookFunc_t unhookFunc = reinterpret_cast<UnhookFunc_t>(GetProcAddress(dll, unhookFuncName.c_str()));
		if (unhookFunc == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		if (!hookFunc(threadId))
			BOOST_THROW_EXCEPTION(Exception() << err_str("挂钩函数失败，详细信息请查看钩子DLL的log文件。"));

		ON_SCOPE_EXIT([unhookFunc]()
		{
			unhookFunc();
		});

		bi::named_mutex hookMutex(bi::open_only, hookMutexName.c_str());
		bi::named_condition hookCond(bi::open_only, hookCondName.c_str());
		bi::scoped_lock<bi::named_mutex> lock(hookMutex);
		hookCond.notify_one();
		hookCond.wait(lock);
	}
}
