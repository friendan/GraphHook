#include "TH10Hook/Common.h"
#include "TH10Hook/TH10Hook.h"

#include <d3d9.h>
#include <atlbase.h>
#include <boost/bind.hpp>
//#include <boost/log/utility/setup/file.hpp>
#include <cpp/ScopeGuard.h>
#include <Windows/Utils.h>
#include <MinHook.h>

#include "TH10Hook/DllMain.h"

namespace th
{
	namespace blog = boost::log;

	typedef IDirect3D9* (WINAPI *Direct3DCreate9_t)(UINT);
	typedef HRESULT(STDMETHODCALLTYPE *CreateDevice_t)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
	typedef HRESULT(STDMETHODCALLTYPE *Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
	typedef HRESULT(STDMETHODCALLTYPE *Present_t)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
	typedef HRESULT(STDMETHODCALLTYPE *BeginScene_t)(IDirect3DDevice9*);
	typedef HRESULT(STDMETHODCALLTYPE *EndScene_t)(IDirect3DDevice9*);
	typedef HRESULT(STDMETHODCALLTYPE *Clear_t)(IDirect3DDevice9*, DWORD, CONST D3DRECT*, DWORD, D3DCOLOR, float, DWORD);

	Reset_t resetTarget = nullptr, resetOrig = nullptr;
	Present_t presentTarget = nullptr, presentOrig = nullptr;
	BeginScene_t beginSceneTarget = nullptr, beginSceneOrig = nullptr;
	EndScene_t endSceneTarget = nullptr, endSceneOrig = nullptr;
	Clear_t clearTarget = nullptr, clearOrig = nullptr;

	HRESULT STDMETHODCALLTYPE ResetHook(IDirect3DDevice9* d3dDevice9, D3DPRESENT_PARAMETERS* presentationParameters)
	{
		HRESULT hr = resetOrig(d3dDevice9, presentationParameters);
		return hr;
	}

	HRESULT STDMETHODCALLTYPE PresentHook(IDirect3DDevice9* d3dDevice9, CONST RECT* sourceRect,
		CONST RECT* destRect, HWND destWindowOverride, CONST RGNDATA* dirtyRegion)
	{
		HRESULT hr = presentOrig(d3dDevice9, sourceRect, destRect, destWindowOverride, dirtyRegion);
		return hr;
	}

	HRESULT STDMETHODCALLTYPE BeginSceneHook(IDirect3DDevice9* d3dDevice9)
	{
		HRESULT hr = beginSceneOrig(d3dDevice9);
		return hr;
	}

	HRESULT STDMETHODCALLTYPE EndSceneHook(IDirect3DDevice9* d3dDevice9)
	{
		HRESULT hr = endSceneOrig(d3dDevice9);
		return hr;
	}

	HRESULT STDMETHODCALLTYPE ClearHook(IDirect3DDevice9* d3dDevice9, DWORD count,
		CONST D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
	{
		HRESULT hr = clearOrig(d3dDevice9, count, rects, flags, color, z, stencil);
		return hr;
	}

	TH10Hook::TH10Hook() :
		m_quit(false)
	{
		DWORD threadId = GetCurrentThreadId();
		HANDLE dllMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);

		m_thread = boost::thread(boost::bind(&TH10Hook::hookProc, this,
			boost::placeholders::_1), dllMainThread);
	}

	TH10Hook::~TH10Hook()
	{
		{
			boost::lock_guard<boost::mutex> lock(m_mutex);
			m_quit = true;
		}
		if (m_thread.joinable())
			m_thread.join();
	}

	void TH10Hook::hookProc(HANDLE dllMainThread)
	{
		WaitForSingleObject(dllMainThread, INFINITE);
		CloseHandle(dllMainThread);

		try
		{
			//std::string logName = win::Utils::GetModuleDir(g_dllModule) + "\\TH10Hook.log";
			//blog::add_file_log(logName);

			{
				WNDCLASSEX wcex = {};
				wcex.cbSize = sizeof(wcex);
				wcex.style = CS_HREDRAW | CS_VREDRAW;
				wcex.hInstance = GetModuleHandle(nullptr);
				wcex.lpfnWndProc = &DefWindowProc;
				wcex.lpszClassName = _T("TH10HookClass");
				if (RegisterClassEx(&wcex) == 0)
					THROW_SYSTEM_EXCEPTION(GetLastError());

				HWND window = CreateWindowEx(0, wcex.lpszClassName, _T("TH10Hook"), WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, wcex.hInstance, nullptr);
				if (window == nullptr)
					THROW_SYSTEM_EXCEPTION(GetLastError());

				ON_SCOPE_EXIT([window]()
				{
					DestroyWindow(window);
				});

				HMODULE d3d9Dll = GetModuleHandle(_T("d3d9.dll"));
				if (d3d9Dll == nullptr)
					THROW_SYSTEM_EXCEPTION(GetLastError());

				Direct3DCreate9_t direct3DCreate9 = reinterpret_cast<Direct3DCreate9_t>(
					GetProcAddress(d3d9Dll, "Direct3DCreate9"));
				if (direct3DCreate9 == nullptr)
					THROW_SYSTEM_EXCEPTION(GetLastError());

				HRESULT hr;

				CComPtr<IDirect3D9> d3d9 = direct3DCreate9(D3D_SDK_VERSION);
				if (d3d9 == nullptr)
					BOOST_THROW_EXCEPTION(Exception() << err_str("Direct3DCreate9() failed."));

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

				//resetTarget = reinterpret_cast<Reset_t>(vtable[16]);
				presentTarget = reinterpret_cast<Present_t>(vtable[17]);
				//beginSceneTarget = reinterpret_cast<BeginScene_t>(vtable[41]);
				//endSceneTarget = reinterpret_cast<EndScene_t>(vtable[42]);
				//clearTarget = reinterpret_cast<Clear_t>(vtable[43]);
			}

			if (MH_Initialize() != MH_OK)
			{
				return;
			}

			//MH_CreateHook(resetTarget, &ResetHook, reinterpret_cast<LPVOID*>(&resetOrig));
			MH_CreateHook(presentTarget, &PresentHook, reinterpret_cast<LPVOID*>(&presentOrig));
			//MH_CreateHook(beginSceneTarget, &BeginSceneHook, reinterpret_cast<LPVOID*>(&beginSceneOrig));
			//MH_CreateHook(endSceneTarget, &EndSceneHook, reinterpret_cast<LPVOID*>(&endSceneOrig));
			//MH_CreateHook(clearTarget, &ClearHook, reinterpret_cast<LPVOID*>(&clearOrig));

			//MH_EnableHook(resetTarget);
			MH_EnableHook(presentTarget);
			//MH_EnableHook(beginSceneTarget);
			//MH_EnableHook(endSceneTarget);
			//MH_EnableHook(clearTarget);

			boost::unique_lock<boost::mutex> lock(m_mutex);
			while (!m_quit)
				m_cv.wait(lock);

			//MH_DisableHook(resetTarget);
			MH_DisableHook(presentTarget);
			//MH_DisableHook(beginSceneTarget);
			//MH_DisableHook(endSceneTarget);
			//MH_DisableHook(clearTarget);

			if (MH_Uninitialize() != MH_OK)
			{
				return;
			}
		}
		catch (...)
		{
			std::string info = boost::current_exception_diagnostic_information();
			//BOOST_LOG_TRIVIAL(error) << info;
		}
	}
}
