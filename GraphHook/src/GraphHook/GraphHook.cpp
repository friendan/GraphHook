#include "GraphHook/Common.h"
#include "GraphHook/GraphHook.h"

#include <boost/log/utility/setup/file.hpp>
#include <Windows/Event.h>

#include "GraphHook/DllMain.h"

namespace gh
{
#define GH_HOOK			(WM_USER + 0x1234)
#define GH_UNHOOK		(WM_USER + 0x1235)
#define GH_TH10HOOK		0x0001

	namespace bl = boost::log;

	GraphHook::GraphHook() :
		Singleton(this),
		m_isUnicode(false),
		m_oldWndProc(nullptr)
	{
	}

	void GraphHook::attach()
	{
		std::string logName = Utils::GetModuleDir(g_dllModule) + "\\GraphHook.log";
		bl::add_file_log(logName);

		try
		{
			m_target = Window::FindByProcessId(GetCurrentProcessId());
			m_isUnicode = m_target.isUnicode();

			if (m_isUnicode)
			{
				LONG_PTR newLong = reinterpret_cast<LONG_PTR>(&GraphHook::NewWndProc);
				LONG_PTR oldLong = SetWindowLongPtrW(m_target, GWLP_WNDPROC, newLong);
				m_oldWndProc = reinterpret_cast<WNDPROC>(oldLong);
			}
			else
			{
				LONG_PTR newLong = reinterpret_cast<LONG_PTR>(&GraphHook::NewWndProc);
				LONG_PTR oldLong = SetWindowLongPtrA(m_target, GWLP_WNDPROC, newLong);
				m_oldWndProc = reinterpret_cast<WNDPROC>(oldLong);
			}

			win::Event hookEvent = win::Event::Open("GraphHookEvent");
			hookEvent.set();
		}
		catch (...)
		{
			std::string what = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << what;
		}
	}

	void GraphHook::detach()
	{
		try
		{
			if (m_isUnicode)
			{
				LONG_PTR oldLong = reinterpret_cast<LONG_PTR>(m_oldWndProc);
				SetWindowLongPtrW(m_target, GWLP_WNDPROC, oldLong);
				m_oldWndProc = nullptr;
			}
			else
			{
				LONG_PTR oldLong = reinterpret_cast<LONG_PTR>(m_oldWndProc);
				SetWindowLongPtrA(m_target, GWLP_WNDPROC, oldLong);
				m_oldWndProc = nullptr;
			}

			//win::Event unhookEvent = win::Event::Open("GraphUnhookEvent");
			//unhookEvent.set();
		}
		catch (...)
		{
			std::string what = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << what;
		}
	}

	void GraphHook::exit()
	{
		try
		{
			HANDLE thread = CreateThread(nullptr, 0, &GraphHook::exitProc, nullptr, 0, nullptr);
			if (thread == nullptr)
				THROW_WINDOWS_EXCEPTION(GetLastError());
			CloseHandle(thread);
		}
		catch (...)
		{
			std::string what = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << what;
		}
	}

	DWORD GraphHook::exitProc(LPVOID)
	{
		Sleep(100);
		FreeLibraryAndExitThread(g_dllModule, 0);
		return 0;
	}

	LRESULT GraphHook::NewWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		GraphHook& graphHook = GraphHook::GetInstance();
		return graphHook.newWndProc(window, msg, wParam, lParam);
	}

	LRESULT GraphHook::newWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case GH_HOOK:
			try
			{
				if (m_minHook == nullptr)
					m_minHook = std::make_shared<MinHookIniter>();
				if ((wParam & GH_TH10HOOK) && m_th10Hook == nullptr)
					m_th10Hook = std::make_shared<TH10Hook>();
			}
			catch (...)
			{
				std::string what = boost::current_exception_diagnostic_information();
				BOOST_LOG_TRIVIAL(error) << what;
			}
			break;

		case GH_UNHOOK:
		{
			m_th10Hook = nullptr;
			m_minHook = nullptr;

			LRESULT lr = defWndProc(window, msg, wParam, lParam);
			detach();
			exit();
			return lr;
		}

		case WM_DESTROY:
			if (window == m_target)
			{
				m_th10Hook = nullptr;
				m_minHook = nullptr;
			}
			break;
		}

		return defWndProc(window, msg, wParam, lParam);
	}

	LRESULT GraphHook::defWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (m_isUnicode)
			return CallWindowProcW(m_oldWndProc, window, msg, wParam, lParam);
		else
			return CallWindowProcA(m_oldWndProc, window, msg, wParam, lParam);
	}
}
