#include "DllInjector/Common.h"
#include "DllInjector/DllInjector.h"

#include <boost/locale.hpp>
#include <cpp/ScopeGuard.h>

namespace di
{
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
}
