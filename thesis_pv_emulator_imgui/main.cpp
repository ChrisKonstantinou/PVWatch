#include "imgui.h"
#include "implot.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>

#include "pv/include/pv.h"
#include <iostream>

// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

int main(int, char**);

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Public HEAP PV objects
PV::PVModule pvModule;
PV::PVModule pvModuleNominal;

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"PV Emulator Interface",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        1280,
        800,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

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
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Setup ImPlot Context
    ImPlot::CreateContext();

    // Our state
    
    bool show_nominal_curves = false;

    bool show_examples = false;

    ImVec4 clear_color = ImVec4(0.08f, 0.20f, 0.27f, 1.00f);

    // PV parameters
    float v_oc = 35.0;
    float i_sc = 9;
    float v_mp = 30.0;
    float i_mp = 8.5;

    float g     = PV::G_nominal;
    float t_e   = PV::T_nominal;

    int voltage_steps = 0; // WARNING: This must be always zero as an initial value
    int prev_voltage_steps = 0;
    int iterrations = 50;

    // Main loop
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
            ResetDevice();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        if (show_examples)
        {
            ImGui::ShowDemoWindow(&show_examples);
            ImPlot::ShowDemoWindow(&show_examples);
        }

        // ---------------------------------------------------------------------------------------------------
        // ---------------------------------------------------------------------------------------------------
        // ---------------------------------------------------------------------------------------------------
        // PARAMETER PANEL
        ImGui::Begin("Input Parameters");

        ImGui::SeparatorText("PV Static IV Params");
        ImGui::InputScalar("Voltage (OC) (V)", ImGuiDataType_Float, &v_oc, NULL);
        ImGui::InputScalar("Current (SC) (A)", ImGuiDataType_Float, &i_sc, NULL);
        ImGui::InputScalar("Voltage (MP) (V)", ImGuiDataType_Float, &v_mp, NULL);
        ImGui::InputScalar("Current (MP) (A)", ImGuiDataType_Float, &i_mp, NULL);

        ImGui::SeparatorText("PV Enviromental Params");
        ImGui::InputScalar("Irradiance (G) (W/m2)", ImGuiDataType_Float, &g, NULL);
        ImGui::InputScalar("Temperature (T) (C)", ImGuiDataType_Float, &t_e, NULL);

        ImGui::SeparatorText("Method Params");
        ImGui::InputScalar("Voltage Steps", ImGuiDataType_S32, &voltage_steps, NULL);
        ImGui::InputScalar("Iterrations / Step", ImGuiDataType_S32, &iterrations, NULL);

        ImGui::Separator();
        ImGui::AlignTextToFramePadding();
        if (ImGui::Button("Plot"))
        {
            pvModule.CalculateIVPArrays(v_oc, i_sc, v_mp, i_mp, g, t_e, voltage_steps, iterrations);
            prev_voltage_steps = voltage_steps;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            voltage_steps = 0;
            prev_voltage_steps = 0;
            pvModule.ClearCurrentArray();
        }

        ImGui::SameLine();
        ImGui::Button("EXPORT plot");

        ImGui::SeparatorText("GUI Settings");
        if (ImGui::Checkbox("Show Nominal Curves", &show_nominal_curves))
        {
            // Create the nominal curves
            pvModuleNominal.CalculateIVPArrays(
                v_oc,
                i_sc,
                v_mp,
                i_mp,
                PV::G_nominal,
                PV::T_nominal,
                PV::STEPS_nominal,
                PV::ITERS_nominal
            );
        }

        //ImGui::ColorEdit3("Background Color", (float*)&clear_color);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        ImGui::End();

        // RENDER CURRENT VOLTAGE PLOT
        ImGui::Begin("Current - Voltage Plot");

        if (ImPlot::BeginPlot("I-V Plot", ImVec2(-1, -1)))
        {

            ImPlot::SetupAxes("Voltage (V)", "Current (A)");
            ImPlot::SetupAxesLimits(0, 1.1 * v_oc, 0, 1.1 * i_sc);

            ImPlot::TagX(v_mp, ImVec4(0, 1, 1, 1), "%s", "Vmp");
            ImPlot::TagY(i_mp, ImVec4(0, 1, 1, 1), "%s", "Imp");

            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.20f);

            ImPlot::PlotShaded("I-V plot", pvModule.GetVoltageArray(), pvModule.GetCurrentArray(), prev_voltage_steps);
            ImPlot::PlotLine("I-V plot", pvModule.GetVoltageArray(), pvModule.GetCurrentArray(), prev_voltage_steps);

            if (show_nominal_curves)
            {
                ImPlot::PlotShaded("I-V plot Nominal", pvModuleNominal.GetVoltageArray(), pvModuleNominal.GetCurrentArray(), PV::STEPS_nominal);
                ImPlot::PlotLine("I-V plot Nominal", pvModuleNominal.GetVoltageArray(), pvModuleNominal.GetCurrentArray(), PV::STEPS_nominal);
            }

            ImPlot::EndPlot();
        }
        ImGui::End();

        // RENDER POWER VOLTAGE PLOT
        ImGui::Begin("Power - Voltage Plot");
        if (ImPlot::BeginPlot("P-V Plot", ImVec2(-1, -1)))
        {

            ImPlot::SetupAxes("Voltage (V)", "Power (W)");
            ImPlot::SetupAxesLimits(0, 1.1 * v_oc, 0, 1.1 * i_sc * v_oc);

            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.20f);

            ImPlot::PlotShaded("P-V plot", pvModule.GetVoltageArray(), pvModule.GetPowerArray(), prev_voltage_steps);
            ImPlot::PlotLine("P-V plot", pvModule.GetVoltageArray(), pvModule.GetPowerArray(), prev_voltage_steps);

            if (show_nominal_curves)
            {
                ImPlot::PlotShaded("P-V plot Nominal", pvModuleNominal.GetVoltageArray(), pvModuleNominal.GetPowerArray(), PV::STEPS_nominal);
                ImPlot::PlotLine("P-V plot Nominal", pvModuleNominal.GetVoltageArray(), pvModuleNominal.GetPowerArray(), PV::STEPS_nominal);
            }

            ImPlot::EndPlot();
        }
        ImGui::End();

        // ---------------------------------------------------------------------------------------------------
        // ---------------------------------------------------------------------------------------------------
        // ---------------------------------------------------------------------------------------------------
        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
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
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    ImPlot::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

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

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
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
