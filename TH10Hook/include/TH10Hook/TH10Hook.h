#pragma once

#include <boost/thread.hpp>

namespace th
{
	class TH10Hook :
		public Object
	{
	public:
		TH10Hook();
		~TH10Hook();

	private:
		void hookProc(HANDLE dllMainThread);

		boost::thread m_thread;
		boost::mutex m_mutex;
		boost::condition_variable m_cv;
		bool m_quit;
	};
}
