#include "GraphHook/Common.h"
#include "GraphHook/GraphHook.h"

#include <boost/log/utility/setup/file.hpp>
#include <Windows/Event.h>

#include "GraphHook/DllMain.h"

namespace gh
{
#define WM_GRAPHHOOK	(WM_USER + 0x1234)
#define GH_EXIT			0x1235
#define GH_TH10_HOOK	0x1236
#define GH_TH10_UNHOOK	0x1237

	namespace blog = boost::log;

	GraphHook::GraphHook() :
		Singleton(this),
		m_isUnicode(false),
		m_oldWndProc(nullptr)
	{
	}

	void GraphHook::hook()
	{
		std::string logName = Utils::GetModuleDir(g_dllModule) + "\\GraphHook.log";
		blog::add_file_log(logName);
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
			win::Event graphHookEvent = win::Event::Open("GraphHookEvent");
			graphHookEvent.set();
		}
		catch (...)
		{
			std::string what = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << what;
		}
	}

	void GraphHook::unhook()
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
			win::Event graphUnhookEvent = win::Event::Open("GraphUnhookEvent");
			graphUnhookEvent.set();
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
		case WM_GRAPHHOOK:
			try
			{
				switch (wParam)
				{
				case GH_TH10_HOOK:
					if (m_minHook == nullptr)
						m_minHook = std::make_shared<MinHookIniter>();
					if (m_th10Hook == nullptr)
						m_th10Hook = std::make_shared<TH10Hook>();
					break;

				case GH_TH10_UNHOOK:
					m_th10Hook = nullptr;
					break;

				case GH_EXIT:
					m_th10Hook = nullptr;
					m_minHook = nullptr;
					LRESULT lr = defWndProc(window, msg, wParam, lParam);
					unhook();
					exit();
					return lr;
				}
			}
			catch (...)
			{
				std::string what = boost::current_exception_diagnostic_information();
				BOOST_LOG_TRIVIAL(error) << what;
			}
			break;

		case WM_NCDESTROY:
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
