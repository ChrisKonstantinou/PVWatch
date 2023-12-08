#pragma once
#include "imgui.h"
#include "implot.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>

#include "../../pv/include/pv.h"
#include <iostream>
#include <thread>
#include <mutex>

#include "../../async_com/include/async_com.h"

// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

int main(int, char**);

// Forward declarations of helper functions
// bool CreateDeviceD3D(HWND hWnd);
// void CleanupDeviceD3D();
// void ResetDevice();

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Public HEAP PV objects
PV::PVModule pvModule;
PV::PVModule pvModuleNominal;

// Real time rendering value pair and Async Communication
double rt_v[1];
double rt_i[1];
double rt_p[1];

std::mutex mtx;

// Simulation Progress
float sim_progress = 0;

namespace PVWatch
{

	class App
	{
	public:
		App()
		{
            // Create application window
            //ImGui_ImplWin32_EnableDpiAwareness();
            this->wc = { sizeof(this->wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
            ::RegisterClassExW(&this->wc);
            
            this->hwnd = ::CreateWindowW(
                this->wc.lpszClassName,
                L"PV Emulator Interface",
                WS_OVERLAPPEDWINDOW,
                100,
                100,
                1280,
                800,
                nullptr,
                nullptr,
                this->wc.hInstance,
                nullptr
            );

            // Initialize Direct3D
            if (!this->CreateDeviceD3D(hwnd))
            {
                this->CleanupDeviceD3D();
                ::UnregisterClassW(this->wc.lpszClassName, this->wc.hInstance);
                //return 1;
                return;
            }

            // Show the window
            ::ShowWindow(this->hwnd, SW_SHOWDEFAULT);
            ::UpdateWindow(this->hwnd);

            // Setup Dear ImGui context
            // IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
            // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsLight();

            // Setup Platform/Renderer backends
            ImGui_ImplWin32_Init(this->hwnd);
            ImGui_ImplDX9_Init(g_pd3dDevice);

            // Setup ImPlot Context
            ImPlot::CreateContext();

            // Our state

            bool show_real_time_pairs = true;
            bool show_nominal_curves = false;
            bool show_examples = false;
            bool show_simulation_panel = true;

            this->clear_color = ImVec4(0.08f, 0.20f, 0.27f, 1.00f);

            // PV parameters
            float v_oc = 35.0;
            float i_sc = 9;
            float v_mp = 30.0;
            float i_mp = 8.5;

            float g = PV::G_nominal;
            float t_e = PV::T_nominal;

            int voltage_steps = 0; // WARNING: This must be always zero as an initial value
            int prev_voltage_steps = 0;
            int iterrations = 50;

            // Simulation parameters
            float sim_g_start = 800;
            float sim_g_stop = 1000;
            float sim_t_start = 25;
            float sim_t_stop = 40;
            float sim_time_s = 10;
            int sim_steps = 70;

            // Setup Async Communication Thread
            std::thread t(&AsyncCommunication::Test, AsyncCommunication());
            t.detach();

            // Init Simulation Class
            PV::Simulator simulator;
		}

		virtual ~App()
		{
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            ImPlot::DestroyContext();

            CleanupDeviceD3D();
            ::DestroyWindow(this->hwnd);
            ::UnregisterClassW(this->wc.lpszClassName, this->wc.hInstance);
		}

		void Run()
		{
			StartUp();

            bool done = false;
			while (!done)
			{
                // Poll and handle messages (inputs, window resize, etc.)
                // See the WndProc() function below for our to dispatch events to the Win32 backend.
                MSG msg;
                while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
                {
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);
                    if (msg.message == WM_QUIT)
                        done = true;
                }
                if (done)
                    break;

                // Handle window resize (we don't resize directly in the WM_SIZE handler)
                if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
                {
                    g_d3dpp.BackBufferWidth = g_ResizeWidth;
                    g_d3dpp.BackBufferHeight = g_ResizeHeight;
                    g_ResizeWidth = g_ResizeHeight = 0;
                    this->ResetDevice();
                }

                // Start the Dear ImGui frame
                ImGui_ImplDX9_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
                ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

				Update();

                ImGui::EndFrame();
                g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
                g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
                D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(
                    (int)(this->clear_color.x * this->clear_color.w * 255.0f),
                    (int)(this->clear_color.y * this->clear_color.w * 255.0f),
                    (int)(this->clear_color.z * this->clear_color.w * 255.0f),
                    (int)(this->clear_color.w * 255.0f)
                );

                g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
                if (g_pd3dDevice->BeginScene() >= 0)
                {
                    ImGui::Render();
                    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                    g_pd3dDevice->EndScene();
                }
                HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

                // Handle loss of D3D9 device
                if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
                    this->ResetDevice();
			}
		}

		virtual void Update() = 0;
		virtual void StartUp() = 0;

	private:
        WNDCLASSEXW wc;
        HWND hwnd;

        ImVec4 clear_color;

        bool CreateDeviceD3D(HWND hWnd)
        {
            if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
                return false;

            // Create the D3DDevice
            ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
            g_d3dpp.Windowed = TRUE;
            g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
            g_d3dpp.EnableAutoDepthStencil = TRUE;
            g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
            g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
            //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
            if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
                return false;

            return true;
        }

        void CleanupDeviceD3D()
        {
            if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
            if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
        }

        void ResetDevice()
        {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
            if (hr == D3DERR_INVALIDCALL)
                IM_ASSERT(0);
            ImGui_ImplDX9_CreateDeviceObjects();
        }
	};
}
