#include "DllInjector/Common.h"
#include "DllInjector/DllInjector.h"

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/locale.hpp>
#include <cpp/ScopeGuard.h>

namespace di
{
	namespace bi = boost::interprocess;
	namespace bl = boost::locale;

	void DllInjector::Inject(Process& target, const std::string& dllPath)
	{
		std::wstring dllPathW = bl::conv::utf_to_utf<wchar_t>(dllPath);
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
			BOOST_THROW_EXCEPTION(Exception() << err_str("written != dllPathSize"));

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

	void DllInjector::Hook(const std::string& dllPath, const std::string& hookFuncName,
		const std::string& unhookFuncName, const std::string& className,
		const std::string& windowName, const std::string& namedMutexName,
		const std::string& namedCondName)
	{
		std::wstring dllPathW = bl::conv::utf_to_utf<wchar_t>(dllPath);
		std::wstring classNameW = bl::conv::utf_to_utf<wchar_t>(className);
		std::wstring windowNameW = bl::conv::utf_to_utf<wchar_t>(windowName);

		HMODULE dll = LoadLibrary(dllPathW.c_str());
		if (dll == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([dll]()
		{
			FreeLibrary(dll);
		});

		typedef HHOOK(WINAPI *HookFunc_t)(LPCTSTR, LPCTSTR);
		typedef void(WINAPI *UnhookFunc_t)(HHOOK);

		HookFunc_t hookFunc = reinterpret_cast<HookFunc_t>(GetProcAddress(dll, hookFuncName.c_str()));
		if (hookFunc == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		UnhookFunc_t unhookFunc = reinterpret_cast<UnhookFunc_t>(GetProcAddress(dll, unhookFuncName.c_str()));
		if (unhookFunc == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		HHOOK hook = hookFunc(classNameW.empty() ? nullptr : classNameW.c_str(),
			windowNameW.empty() ? nullptr : windowNameW.c_str());

		ON_SCOPE_EXIT([&]()
		{
			unhookFunc(hook);
		});

		bi::named_mutex namedMutex(bi::create_only, namedMutexName.c_str());
		bi::named_condition namedCond(bi::create_only, namedCondName.c_str());
		bi::scoped_lock<bi::named_mutex> lock(namedMutex);
		namedCond.wait(lock);
	}
}
