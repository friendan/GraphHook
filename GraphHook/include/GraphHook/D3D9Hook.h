#pragma once

#include <d3d9.h>
#include <atlbase.h>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cpp/Singleton.h>
#include <Windows/Event.h>

#include "GraphHook/MinHookFunc.h"

namespace gh
{
	namespace bip = boost::interprocess;

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
		private Singleton<D3D9Hook>
	{
	public:
		D3D9Hook();
		~D3D9Hook();

	private:
		static HRESULT STDMETHODCALLTYPE ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentationParameters);
		static HRESULT STDMETHODCALLTYPE PresentHook(IDirect3DDevice9* device, CONST RECT* sourceRect, CONST RECT* destRect,
			HWND destWindowOverride, CONST RGNDATA* dirtyRegion);
		static HRESULT STDMETHODCALLTYPE BeginSceneHook(IDirect3DDevice9* device);
		static HRESULT STDMETHODCALLTYPE EndSceneHook(IDirect3DDevice9* device);
		static HRESULT STDMETHODCALLTYPE ClearHook(IDirect3DDevice9* device, DWORD count, CONST D3DRECT* rects, DWORD flags,
			D3DCOLOR color, float z, DWORD stencil);

		HRESULT resetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentationParameters);
		HRESULT presentHook(IDirect3DDevice9* device, CONST RECT* sourceRect, CONST RECT* destRect,
			HWND destWindowOverride, CONST RGNDATA* dirtyRegion);
		HRESULT beginSceneHook(IDirect3DDevice9* device);
		HRESULT endSceneHook(IDirect3DDevice9* device);
		HRESULT clearHook(IDirect3DDevice9* device, DWORD count, CONST D3DRECT* rects, DWORD flags,
			D3DCOLOR color, float z, DWORD stencil);

		void capture(IDirect3DDevice9* device);

		MinHookFunc m_presentFunc;

		Reset_t m_resetOrig;
		Present_t m_presentOrig;
		BeginScene_t m_beginSceneOrig;
		EndScene_t m_endSceneOrig;
		Clear_t m_clearOrig;

		win::Event m_presentEvent;

		CComPtr<IDirect3DSurface9> m_destSurface;
		D3DSURFACE_DESC m_desc;
		D3DLOCKED_RECT m_lockedRect;
		bip::windows_shared_memory m_shm;
		bip::mapped_region m_region;
	};
}
