#include "TH10Hook/Common.h"
#include "TH10Hook/D3D9Hook.h"

#include <cpp/ScopeGuard.h>

namespace th
{
	D3D9Hook::D3D9Hook() :
		Singleton(this),
		m_resetOrig(nullptr),
		m_presentOrig(nullptr),
		m_beginSceneOrig(nullptr),
		m_endSceneOrig(nullptr),
		m_clearOrig(nullptr)
	{
	}

	D3D9Hook::~D3D9Hook()
	{
	}

	intptr_t* D3D9Hook::getVTable()
	{
		WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.hInstance = GetModuleHandle(nullptr);
		wcex.lpfnWndProc = &DefWindowProc;
		wcex.lpszClassName = _T("D3D9HookClass");
		if (RegisterClassEx(&wcex) == 0)
			THROW_WINDOWS_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([&wcex]()
		{
			UnregisterClass(wcex.lpszClassName, wcex.hInstance);
		});

		HWND window = CreateWindowEx(0, wcex.lpszClassName, _T("D3D9Hook"), WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, wcex.hInstance, nullptr);
		if (window == nullptr)
			THROW_WINDOWS_EXCEPTION(GetLastError());

		ON_SCOPE_EXIT([window]()
		{
			DestroyWindow(window);
		});

		HMODULE d3d9Dll = GetModuleHandle(_T("d3d9.dll"));
		if (d3d9Dll == nullptr)
			THROW_WINDOWS_EXCEPTION(GetLastError());

		Direct3DCreate9_t direct3DCreate9 = reinterpret_cast<Direct3DCreate9_t>(
			GetProcAddress(d3d9Dll, "Direct3DCreate9"));
		if (direct3DCreate9 == nullptr)
			THROW_WINDOWS_EXCEPTION(GetLastError());

		HRESULT hr;

		CComPtr<IDirect3D9> d3d9 = direct3DCreate9(D3D_SDK_VERSION);
		if (d3d9 == nullptr)
			THROW_CPP_EXCEPTION(Exception() << err_str("Direct3DCreate9() failed."));

		D3DDISPLAYMODE d3ddm = {};
		hr = d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
		if (FAILED(hr))
			THROW_DIRECTX_EXCEPTION(hr);

		D3DPRESENT_PARAMETERS d3dpp = {};
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = d3ddm.Format;

		CComPtr<IDirect3DDevice9> d3dDevice9;
		hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
			&d3dpp, &d3dDevice9);
		if (FAILED(hr))
			THROW_DIRECTX_EXCEPTION(hr);

		intptr_t* vtable = (intptr_t*)(*((intptr_t*)d3dDevice9.p));
		return vtable;
	}

	void D3D9Hook::hook()
	{
		m_presentEvent = win::Event::Open("D3DPresentEvent");

		intptr_t* vtable = getVTable();
		//m_resetTarget = reinterpret_cast<Reset_t>(vtable[16]);
		//m_presentTarget = reinterpret_cast<Present_t>(vtable[17]);
		//m_beginSceneTarget = reinterpret_cast<BeginScene_t>(vtable[41]);
		//m_endSceneTarget = reinterpret_cast<EndScene_t>(vtable[42]);
		//m_clearTarget = reinterpret_cast<Clear_t>(vtable[43]);

		//MH_CreateHook(m_resetTarget, &ResetHook, reinterpret_cast<LPVOID*>(&m_resetOrig));
		m_presentFunc = MinHookFunc(reinterpret_cast<LPVOID>(vtable[17]), &D3D9Hook::PresentHook, reinterpret_cast<LPVOID*>(&m_presentOrig));
		//MH_CreateHook(m_beginSceneTarget, &BeginSceneHook, reinterpret_cast<LPVOID*>(&m_beginSceneOrig));
		//MH_CreateHook(m_endSceneTarget, &EndSceneHook, reinterpret_cast<LPVOID*>(&m_endSceneOrig));
		//MH_CreateHook(m_clearTarget, &ClearHook, reinterpret_cast<LPVOID*>(&m_clearOrig));
	}

	void D3D9Hook::unhook()
	{
		m_presentFunc = MinHookFunc();

		m_presentEvent = win::Event();
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::ResetHook(IDirect3DDevice9* d3dDevice9, D3DPRESENT_PARAMETERS* presentationParameters)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.resetHook(d3dDevice9, presentationParameters);
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::PresentHook(IDirect3DDevice9* d3dDevice9, CONST RECT* sourceRect, CONST RECT* destRect,
		HWND destWindowOverride, CONST RGNDATA* dirtyRegion)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.presentHook(d3dDevice9, sourceRect, destRect, destWindowOverride, dirtyRegion);
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::BeginSceneHook(IDirect3DDevice9* d3dDevice9)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.beginSceneHook(d3dDevice9);
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::EndSceneHook(IDirect3DDevice9* d3dDevice9)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.endSceneHook(d3dDevice9);
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::ClearHook(IDirect3DDevice9* d3dDevice9, DWORD count, CONST D3DRECT* rects, DWORD flags,
		D3DCOLOR color, float z, DWORD stencil)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.clearHook(d3dDevice9, count, rects, flags, color, z, stencil);
	}

	HRESULT D3D9Hook::resetHook(IDirect3DDevice9* d3dDevice9, D3DPRESENT_PARAMETERS* presentationParameters)
	{
		HRESULT hr = m_resetOrig(d3dDevice9, presentationParameters);
		return hr;
	}

	HRESULT D3D9Hook::presentHook(IDirect3DDevice9* d3dDevice9, CONST RECT* sourceRect, CONST RECT* destRect,
		HWND destWindowOverride, CONST RGNDATA* dirtyRegion)
	{
		//m_presentEvent.set();
		SetEvent(m_presentEvent);

		HRESULT hr = m_presentOrig(d3dDevice9, sourceRect, destRect, destWindowOverride, dirtyRegion);

		return hr;
	}

	HRESULT D3D9Hook::beginSceneHook(IDirect3DDevice9* d3dDevice9)
	{
		HRESULT hr = m_beginSceneOrig(d3dDevice9);
		return hr;
	}

	HRESULT D3D9Hook::endSceneHook(IDirect3DDevice9* d3dDevice9)
	{
		HRESULT hr = m_endSceneOrig(d3dDevice9);
		return hr;
	}

	HRESULT D3D9Hook::clearHook(IDirect3DDevice9* d3dDevice9, DWORD count, CONST D3DRECT* rects, DWORD flags,
		D3DCOLOR color, float z, DWORD stencil)
	{
		HRESULT hr = m_clearOrig(d3dDevice9, count, rects, flags, color, z, stencil);
		return hr;
	}
}
