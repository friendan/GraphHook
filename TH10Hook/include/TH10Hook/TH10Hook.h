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
		void HookProc(HANDLE dllMainThread);

		boost::thread s_hookThread;
	};
}
