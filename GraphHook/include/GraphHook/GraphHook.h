#pragma once

#include <memory>
#include <cpp/Singleton.h>
#include <Windows/Window.h>

#include "GraphHook/MinHookIniter.h"
#include "GraphHook/TH10Hook.h"
#include "GraphHook/D3D9Hook.h"

namespace gh
{
	class GraphHook :
		private Singleton<GraphHook>
	{
	public:
		GraphHook();

		void attach();
		void detach();

	private:
		static DWORD WINAPI exitProc(LPVOID);
		static LRESULT CALLBACK NewWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT newWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT defWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

		Window m_target;
		bool m_isUnicode;
		WNDPROC m_oldWndProc;

		std::shared_ptr<MinHookIniter> m_minHook;
		std::shared_ptr<TH10Hook> m_th10Hook;
		std::shared_ptr<D3D9Hook> m_d3d9Hook;
	};
}
