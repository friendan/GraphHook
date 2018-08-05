#pragma once

namespace th
{
	class MinHookWrapper
	{
	public:
		MinHookWrapper();
		~MinHookWrapper();

		void hookAll();
		void unhookAll();

	private:
		bool m_enabled;
	};
}
