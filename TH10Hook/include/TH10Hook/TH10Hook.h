#pragma once

#include <boost/thread.hpp>
#include <boost/atomic.hpp>

namespace th
{
	class TH10Hook
	{
	public:
		TH10Hook();
		~TH10Hook();

		void hookProc(int code, WPARAM wParam, LPARAM lParam);

	private:
		void startHook();
		void stopHook();
		void hookProc(HANDLE dllMainThread);

		boost::thread m_thread;
		boost::mutex m_mutex;
		boost::condition_variable m_cv;
		bool m_quit;
	};
}
