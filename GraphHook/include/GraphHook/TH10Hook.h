#pragma once

#include <d3d9.h>
#include <atlbase.h>
#include <cpp/Singleton.h>
#include <Windows/Event.h>

#include "GraphHook/MinHookFunc.h"

namespace gh
{
	// IDirect3D9
	typedef IDirect3D9* (WINAPI *Direct3DCreate9_t)(UINT);

	// IDirect3DDevice9
	typedef HRESULT(STDMETHODCALLTYPE *Present_t)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);

	class TH10Hook :
		private Singleton<TH10Hook>
	{
	public:
		TH10Hook();
		~TH10Hook();

	private:
		static HRESULT STDMETHODCALLTYPE PresentHook(IDirect3DDevice9* d3dDevice9, CONST RECT* sourceRect, CONST RECT* destRect,
			HWND destWindowOverride, CONST RGNDATA* dirtyRegion);

		HRESULT presentHook(IDirect3DDevice9* d3dDevice9, CONST RECT* sourceRect, CONST RECT* destRect,
			HWND destWindowOverride, CONST RGNDATA* dirtyRegion);

		MinHookFunc m_presentFunc;
		Present_t m_presentOrig;
		win::Event m_presentEvent;
	};
}
