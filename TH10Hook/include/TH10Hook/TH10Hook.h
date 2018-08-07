#pragma once

#include <boost/thread.hpp>
#include <cpp/Singleton.h>

namespace th
{
	class TH10Hook :
		public Singleton<TH10Hook>
	{
	public:
		TH10Hook();
		~TH10Hook();

		bool hook(HWND window);
		void unhook();

	private:
		static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
		LRESULT hookProc(int code, WPARAM wParam, LPARAM lParam);

		void startHook();
		void stopHook();
		void hookProc(HANDLE dllMainThread);

		boost::thread m_thread;
		boost::mutex m_mutex;
		boost::condition_variable m_cv;
		bool m_quit;
	};
}
