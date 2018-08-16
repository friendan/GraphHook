#pragma once

#include <d3d9.h>
#include <atlbase.h>
#include <cpp/Singleton.h>
#include <Windows/Event.h>

#include "TH10Hook/MinHookIniter.h"
#include "TH10Hook/MinHookFunc.h"

namespace th
{
	// IDirect3D9
	typedef IDirect3D9* (WINAPI *Direct3DCreate9_t)(UINT);
	typedef HRESULT(STDMETHODCALLTYPE *CreateDevice_t)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);

	// IDirect3DDevice9
	typedef HRESULT(STDMETHODCALLTYPE *Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
	typedef HRESULT(STDMETHODCALLTYPE *Present_t)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
	typedef HRESULT(STDMETHODCALLTYPE *BeginScene_t)(IDirect3DDevice9*);
	typedef HRESULT(STDMETHODCALLTYPE *EndScene_t)(IDirect3DDevice9*);
	typedef HRESULT(STDMETHODCALLTYPE *Clear_t)(IDirect3DDevice9*, DWORD, CONST D3DRECT*, DWORD, D3DCOLOR, float, DWORD);

	class D3D9Hook :
		public Singleton<D3D9Hook>
	{
	public:
		D3D9Hook();
		~D3D9Hook();

		void hook();
		void unhook();

	private:
		static HRESULT STDMETHODCALLTYPE ResetHook(IDirect3DDevice9* d3dDevice9, D3DPRESENT_PARAMETERS* presentationParameters);
		static HRESULT STDMETHODCALLTYPE PresentHook(IDirect3DDevice9* d3dDevice9, CONST RECT* sourceRect, CONST RECT* destRect,
			HWND destWindowOverride, CONST RGNDATA* dirtyRegion);
		static HRESULT STDMETHODCALLTYPE BeginSceneHook(IDirect3DDevice9* d3dDevice9);
		static HRESULT STDMETHODCALLTYPE EndSceneHook(IDirect3DDevice9* d3dDevice9);
		static HRESULT STDMETHODCALLTYPE ClearHook(IDirect3DDevice9* d3dDevice9, DWORD count, CONST D3DRECT* rects, DWORD flags,
			D3DCOLOR color, float z, DWORD stencil);

		HRESULT resetHook(IDirect3DDevice9* d3dDevice9, D3DPRESENT_PARAMETERS* presentationParameters);
		HRESULT presentHook(IDirect3DDevice9* d3dDevice9, CONST RECT* sourceRect, CONST RECT* destRect,
			HWND destWindowOverride, CONST RGNDATA* dirtyRegion);
		HRESULT beginSceneHook(IDirect3DDevice9* d3dDevice9);
		HRESULT endSceneHook(IDirect3DDevice9* d3dDevice9);
		HRESULT clearHook(IDirect3DDevice9* d3dDevice9, DWORD count, CONST D3DRECT* rects, DWORD flags,
			D3DCOLOR color, float z, DWORD stencil);

		intptr_t* getVTable();

		MinHookIniter m_minHook;
		MinHookFunc m_presentFunc;

		Reset_t m_resetOrig;
		Present_t m_presentOrig;
		BeginScene_t m_beginSceneOrig;
		EndScene_t m_endSceneOrig;
		Clear_t m_clearOrig;

		win::Event m_presentEvent;
	};
}
