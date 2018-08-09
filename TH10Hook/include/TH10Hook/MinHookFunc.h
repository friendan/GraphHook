#pragma once

namespace th
{
	class MinHookFunc
	{
	public:
		MinHookFunc();
		MinHookFunc(LPVOID target, LPVOID detour, LPVOID* original);
		MinHookFunc(const MinHookFunc&) = delete;
		MinHookFunc(MinHookFunc&& other);
		~MinHookFunc();
		MinHookFunc& operator =(const MinHookFunc&) = delete;
		MinHookFunc& operator =(MinHookFunc&& other);
		void swap(MinHookFunc& other);

	private:
		LPVOID m_target;
	};
}
