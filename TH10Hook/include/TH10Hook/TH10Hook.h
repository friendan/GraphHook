#pragma once

#include <memory>
#include <cpp/Singleton.h>

#include "TH10Hook/D3D9Hook.h"

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
		void hookD3D();
		void unhookD3D();

		std::shared_ptr<D3D9Hook> m_d3d9Hook;
	};
}
