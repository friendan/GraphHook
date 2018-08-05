#pragma once

namespace th
{
	class MinHookWrapper
	{
	public:
		MinHookWrapper();
		~MinHookWrapper();

		void enableAll();
		void disableAll();

	private:
		bool m_enabled;
	};
}
