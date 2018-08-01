#include "DllInjector/Common.h"
#include "DllInjector/DllInjector.h"

#include <boost/locale.hpp>
#include <cpp/ScopeGuard.h>

namespace di
{
	namespace bl = boost::locale;

	void DllInjector::enableDebugPrivilege()
	{
		HANDLE token = nullptr;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([token]()
		{
			CloseHandle(token);
		});

		LUID luid = {};
		if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid))
			THROW_SYSTEM_EXCEPTION(GetLastError());

		TOKEN_PRIVILEGES tp = {};
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr))
			THROW_SYSTEM_EXCEPTION(GetLastError());
		if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
			BOOST_THROW_EXCEPTION(Exception() << err_str("Please run as administrator."));
	}

	void DllInjector::injectDll(Process& process, const std::string& dllPath)
	{
		std::wstring dllPathW = bl::conv::utf_to_utf<wchar_t>(dllPath);
		size_t dllPathSize = (dllPathW.length() + 1) * sizeof(wchar_t);

		LPVOID remoteMemory = VirtualAllocEx(process.get(), nullptr, dllPathSize,
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (remoteMemory == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([&]()
		{
			VirtualFreeEx(process.get(), remoteMemory, dllPathSize, MEM_RELEASE);
		});

		SIZE_T written = 0;
		if (!WriteProcessMemory(process.get(), remoteMemory, dllPathW.c_str(), dllPathSize, &written))
			THROW_SYSTEM_EXCEPTION(GetLastError());
		if (written != dllPathSize)
			BOOST_THROW_EXCEPTION(Exception() << err_str("written != dllPathSize"));

		HMODULE kernel32 = GetModuleHandle(_T("kernel32.dll"));
		if (kernel32 == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		FARPROC loadLibrary = GetProcAddress(kernel32, "LoadLibraryW");
		if (loadLibrary == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		DWORD threadId = 0;
		HANDLE remoteThread = CreateRemoteThread(process.get(), nullptr, 0,
			(LPTHREAD_START_ROUTINE)loadLibrary, remoteMemory, 0, &threadId);
		if (remoteThread == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([remoteThread]()
		{
			CloseHandle(remoteThread);
		});

		WaitForSingleObject(remoteThread, INFINITE);
	}

	void DllInjector::uninjectDll(Process& process, const std::string& dllName)
	{
		HMODULE module = process.findModuleByName(dllName);
		if (module == nullptr)
			return;

		HMODULE kernel32 = GetModuleHandle(_T("kernel32.dll"));
		if (kernel32 == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		FARPROC freeLibrary = GetProcAddress(kernel32, "FreeLibrary");
		if (freeLibrary == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		DWORD threadId = 0;
		HANDLE remoteThread = CreateRemoteThread(process.get(), nullptr, 0,
			(LPTHREAD_START_ROUTINE)freeLibrary, module, 0, &threadId);
		if (remoteThread == nullptr)
			THROW_SYSTEM_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([remoteThread]()
		{
			CloseHandle(remoteThread);
		});

		WaitForSingleObject(remoteThread, INFINITE);
	}
}
