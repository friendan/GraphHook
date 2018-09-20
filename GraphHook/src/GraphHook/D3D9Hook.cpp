#include "GraphHook/Common.h"
#include "GraphHook/D3D9Hook.h"

#include <cpp/ScopeGuard.h>

namespace gh
{
	D3D9Hook::D3D9Hook() :
		Singleton(this),
		m_resetOrig(nullptr),
		m_presentOrig(nullptr),
		m_beginSceneOrig(nullptr),
		m_endSceneOrig(nullptr),
		m_clearOrig(nullptr)
	{
		WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = &DefWindowProc;
		wcex.hInstance = GetModuleHandle(nullptr);
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

		CComPtr<IDirect3D9> d3d9;
		d3d9.p = direct3DCreate9(D3D_SDK_VERSION);
		if (d3d9 == nullptr)
			THROW_CPP_EXCEPTION(Exception() << err_str("Direct3DCreate9() failed."));

		D3DDISPLAYMODE mode = {};
		hr = d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);
		if (FAILED(hr))
			THROW_DIRECTX_EXCEPTION(hr);

		D3DPRESENT_PARAMETERS pp = {};
		pp.Windowed = TRUE;
		pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		pp.BackBufferFormat = mode.Format;

		CComPtr<IDirect3DDevice9> device;
		hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
			&pp, &device);
		if (FAILED(hr))
			THROW_DIRECTX_EXCEPTION(hr);

		uintptr_t* vtable = (uintptr_t*)(*((uintptr_t*)device.p));

		m_presentEvent = win::Event::Open("D3D9PresentEvent");

		//MH_CreateHook(reinterpret_cast<Reset_t>(vtable[16]), &ResetHook, reinterpret_cast<LPVOID*>(&m_resetOrig));
		m_presentFunc = MinHookFunc(reinterpret_cast<LPVOID>(vtable[17]), &D3D9Hook::PresentHook, reinterpret_cast<LPVOID*>(&m_presentOrig));
		//MH_CreateHook(reinterpret_cast<BeginScene_t>(vtable[41]), &BeginSceneHook, reinterpret_cast<LPVOID*>(&m_beginSceneOrig));
		//MH_CreateHook(reinterpret_cast<EndScene_t>(vtable[42]), &EndSceneHook, reinterpret_cast<LPVOID*>(&m_endSceneOrig));
		//MH_CreateHook(reinterpret_cast<Clear_t>(vtable[43]), &ClearHook, reinterpret_cast<LPVOID*>(&m_clearOrig));

		win::Event hookEvent = win::Event::Open("D3D9HookEvent");
		hookEvent.set();
	}

	D3D9Hook::~D3D9Hook()
	{
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentationParameters)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.resetHook(device, presentationParameters);
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::PresentHook(IDirect3DDevice9* device, CONST RECT* sourceRect, CONST RECT* destRect,
		HWND destWindowOverride, CONST RGNDATA* dirtyRegion)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.presentHook(device, sourceRect, destRect, destWindowOverride, dirtyRegion);
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::BeginSceneHook(IDirect3DDevice9* device)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.beginSceneHook(device);
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::EndSceneHook(IDirect3DDevice9* device)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.endSceneHook(device);
	}

	HRESULT STDMETHODCALLTYPE D3D9Hook::ClearHook(IDirect3DDevice9* device, DWORD count, CONST D3DRECT* rects, DWORD flags,
		D3DCOLOR color, float z, DWORD stencil)
	{
		D3D9Hook& d3d9Hook = D3D9Hook::GetInstance();
		return d3d9Hook.clearHook(device, count, rects, flags, color, z, stencil);
	}

	HRESULT D3D9Hook::resetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentationParameters)
	{
		HRESULT hr = m_resetOrig(device, presentationParameters);
		return hr;
	}

	HRESULT D3D9Hook::presentHook(IDirect3DDevice9* device, CONST RECT* sourceRect, CONST RECT* destRect,
		HWND destWindowOverride, CONST RGNDATA* dirtyRegion)
	{
		try
		{
			capture(device);
		}
		catch (...)
		{
			std::string what = boost::current_exception_diagnostic_information();
			BOOST_LOG_TRIVIAL(error) << what;
		}

		HRESULT hr = m_presentOrig(device, sourceRect, destRect, destWindowOverride, dirtyRegion);
		return hr;
	}

	HRESULT D3D9Hook::beginSceneHook(IDirect3DDevice9* device)
	{
		HRESULT hr = m_beginSceneOrig(device);
		return hr;
	}

	HRESULT D3D9Hook::endSceneHook(IDirect3DDevice9* device)
	{
		HRESULT hr = m_endSceneOrig(device);
		return hr;
	}

	HRESULT D3D9Hook::clearHook(IDirect3DDevice9* device, DWORD count, CONST D3DRECT* rects, DWORD flags,
		D3DCOLOR color, float z, DWORD stencil)
	{
		HRESULT hr = m_clearOrig(device, count, rects, flags, color, z, stencil);
		return hr;
	}

	void D3D9Hook::capture(IDirect3DDevice9* device)
	{
		HRESULT hr;

		CComPtr<IDirect3DSurface9> renderTarget;
		hr = device->GetRenderTarget(0, &renderTarget);
		if (FAILED(hr))
			THROW_DIRECTX_EXCEPTION(hr);

		if (m_destSurface == nullptr)
		{
			m_desc = {};
			hr = renderTarget->GetDesc(&m_desc);
			if (FAILED(hr))
				THROW_DIRECTX_EXCEPTION(hr);

			hr = device->CreateOffscreenPlainSurface(m_desc.Width, m_desc.Height, m_desc.Format,
				D3DPOOL_SYSTEMMEM, &m_destSurface, nullptr);
			if (FAILED(hr))
				THROW_DIRECTX_EXCEPTION(hr);

			m_lockedRect = {};
			hr = m_destSurface->LockRect(&m_lockedRect, nullptr, D3DLOCK_READONLY);
			if (FAILED(hr))
				THROW_DIRECTX_EXCEPTION(hr);
			ON_SCOPE_EXIT([&]()
			{
				m_destSurface->UnlockRect();
			});

			m_shm = bip::windows_shared_memory(bip::create_only, "D3D9HookSHM", bip::read_write, 1000);
			m_region = bip::mapped_region(m_shm, bip::read_write);
		}

		hr = device->GetRenderTargetData(renderTarget, m_destSurface);
		if (FAILED(hr))
			THROW_DIRECTX_EXCEPTION(hr);

		D3DLOCKED_RECT lockedRect = {};
		hr = m_destSurface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY);
		if (FAILED(hr))
			THROW_DIRECTX_EXCEPTION(hr);
		ON_SCOPE_EXIT([&]()
		{
			m_destSurface->UnlockRect();
		});


	}
}
