#include "GraphHook/Common.h"
#include "GraphHook/TH10Hook.h"

#include <cpp/ScopeGuard.h>

namespace gh
{
	TH10Hook::TH10Hook() :
		Singleton(this),
		m_presentOrig(nullptr)
	{
		WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = &DefWindowProc;
		wcex.hInstance = GetModuleHandle(nullptr);
		wcex.lpszClassName = _T("TH10HookClass");
		if (RegisterClassEx(&wcex) == 0)
			THROW_WINDOWS_EXCEPTION(GetLastError());
		ON_SCOPE_EXIT([&wcex]()
		{
			UnregisterClass(wcex.lpszClassName, wcex.hInstance);
		});

		HWND window = CreateWindowEx(0, wcex.lpszClassName, _T("TH10Hook"), WS_OVERLAPPEDWINDOW,
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

		m_presentEvent = win::Event::Open("TH10PresentEvent");

		m_presentFunc = MinHookFunc(reinterpret_cast<LPVOID>(vtable[17]), &TH10Hook::PresentHook, reinterpret_cast<LPVOID*>(&m_presentOrig));

		win::Event hookEvent = win::Event::Open("TH10HookEvent");
		hookEvent.set();
	}

	TH10Hook::~TH10Hook()
	{
		//try
		//{
		//	win::Event unhookEvent = win::Event::Open("TH10UnhookEvent");
		//	unhookEvent.set();
		//}
		//catch (...)
		//{
		//	std::string what = boost::current_exception_diagnostic_information();
		//	BOOST_LOG_TRIVIAL(info) << what;
		//}
	}

	HRESULT STDMETHODCALLTYPE TH10Hook::PresentHook(IDirect3DDevice9* device, CONST RECT* sourceRect, CONST RECT* destRect,
		HWND destWindowOverride, CONST RGNDATA* dirtyRegion)
	{
		TH10Hook& th10Hook = TH10Hook::GetInstance();
		return th10Hook.presentHook(device, sourceRect, destRect, destWindowOverride, dirtyRegion);
	}

	HRESULT TH10Hook::presentHook(IDirect3DDevice9* device, CONST RECT* sourceRect, CONST RECT* destRect,
		HWND destWindowOverride, CONST RGNDATA* dirtyRegion)
	{
		//m_presentEvent.set();
		SetEvent(m_presentEvent);

		HRESULT hr = m_presentOrig(device, sourceRect, destRect, destWindowOverride, dirtyRegion);

		return hr;
	}
}
