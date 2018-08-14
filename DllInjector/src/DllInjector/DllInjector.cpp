#include "DllInjector/Common.h"
#include "DllInjector/DllInjector.h"

#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <cpp/ScopeGuard.h>
#include <Windows/Event.h>

namespace di
{
	namespace bfs = boost::filesystem;
	namespace blc = boost::locale::conv;

	void DllInjector::Inject(Process& target, const std::string& dllName)
	{
		std::string dllPath = Utils::GetModuleDir() + "\\" + dllName;
		if (!bfs::exists(dllPath))
			BOOST_THROW_EXCEPTION(Exception() << err_str(u8"文件不存在：" + dllPath));
		std::wstring dllPathW = blc::utf_to_utf<wchar_t>(dllPath);
		size_t dllPathSize = (dllPathW.length() + 1) * sizeof(wchar_t);

		LPVOID remoteMemory = VirtualAllocEx(target, nullptr, dllPathSize, MEM_RESERVE | MEM_COMMIT,
			PAGE_READWRITE);
		if (remoteMemory == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([&]()
		{
			VirtualFreeEx(target, remoteMemory, 0, MEM_RELEASE);
		});

		SIZE_T written = 0;
		if (!WriteProcessMemory(target, remoteMemory, dllPathW.c_str(), dllPathSize, &written))
			THROW_SYSTEM_EXCEPTION(GetLastError());
		if (written != dllPathSize)
			BOOST_THROW_EXCEPTION(Exception() << err_str(u8"写入DLL路径的长度错误。"));

		HMODULE kernel32 = GetModuleHandle(_T("kernel32.dll"));
		if (kernel32 == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		FARPROC loadLibrary = GetProcAddress(kernel32, "LoadLibraryW");
		if (loadLibrary == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		NullHandle remoteThread = target.createRemoteThread(nullptr, 0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary), remoteMemory, 0, nullptr);
		WaitForSingleObject(remoteThread, INFINITE);
	}

	void DllInjector::Uninject(Process& target, const std::string& dllName)
	{
		HMODULE module = target.findModuleByName(dllName);
		if (module == nullptr)
			return;

		HMODULE kernel32 = GetModuleHandle(_T("kernel32.dll"));
		if (kernel32 == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		FARPROC freeLibrary = GetProcAddress(kernel32, "FreeLibrary");
		if (freeLibrary == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		NullHandle remoteThread = target.createRemoteThread(nullptr, 0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(freeLibrary), module, 0, nullptr);
		WaitForSingleObject(remoteThread, INFINITE);
	}

	void DllInjector::HookProc(DWORD threadId, const std::string& dllName,
		std::string hookFuncName, std::string unhookFuncName,
		std::string hookEventName, std::string unhookEventName)
	{
		std::string dllPath = Utils::GetModuleDir() + "\\" + dllName;
		if (!bfs::exists(dllPath))
			BOOST_THROW_EXCEPTION(Exception() << err_str(u8"文件不存在：" + dllPath));
		std::wstring dllPathW = blc::utf_to_utf<wchar_t>(dllPath);

		HMODULE dll = LoadLibrary(dllPathW.c_str());
		if (dll == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([dll]()
		{
			FreeLibrary(dll);
		});

		typedef bool(WINAPI *HookFunc_t)(DWORD);
		typedef void(WINAPI *UnhookFunc_t)();

		HookFunc_t hookFunc = reinterpret_cast<HookFunc_t>(GetProcAddress(dll, hookFuncName.c_str()));
		if (hookFunc == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		UnhookFunc_t unhookFunc = reinterpret_cast<UnhookFunc_t>(GetProcAddress(dll, unhookFuncName.c_str()));
		if (unhookFunc == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		if (!hookFunc(threadId))
			BOOST_THROW_EXCEPTION(Exception() << err_str(u8"挂钩失败，详细信息请查看钩子DLL的log文件。"));

		ON_SCOPE_EXIT([unhookFunc]()
		{
			unhookFunc();
		});

		{
			win::Event hookEvent = win::Event::Open(hookEventName);
			hookEvent.set();
		}
		{
			win::Event unhookEvent = win::Event::Create(unhookEventName);
			unhookEvent.wait();
		}
	}
}
