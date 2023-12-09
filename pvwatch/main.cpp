#include "app_design/include/app_design.h"

#include <iostream>
#include <thread>
#include <mutex>

#include "pv/include/pv.h"
#include "async_com/include/async_com.h"


// PV modules
PV::PVModule pvModule;
PV::PVModule pvModuleNominal;

// Real time rendering value pair and Async Communication
double rt_v[1];
double rt_i[1];
double rt_p[1];

// Simulation Progress
float sim_progress = 0;


class PVWatchApp : public App
{
public:
    PVWatchApp() = default;
    ~PVWatchApp() = default;

    // Application state
    bool show_real_time_pairs = true;
    bool show_nominal_curves = false;

    // PV initial parameters
    float v_oc = 35.0;
    float i_sc = 9;
    float v_mp = 30.0;
    float i_mp = 8.5;

    float g = PV::G_nominal;
    float t_e = PV::T_nominal;

    int voltage_steps = 0; // WARNING: This must be always zero as an initial value
    int prev_voltage_steps = 0;
    int iterrations = 50;

    // Simulation initial parameters
    float sim_g_start = 800;
    float sim_g_stop = 1000;
    float sim_t_start = 25;
    float sim_t_stop = 40;
    float sim_time_s = 10;
    int sim_steps = 70;

    // Init Simulation Class
    PV::Simulator simulator;

    virtual void StartUp() final
    {
        // Startup Async Communication Thread
        std::thread t(&AsyncCommunication::Test, AsyncCommunication());
        t.detach();
    }

    virtual void Update() final
    {
        if (show_demo_windows)
        {
            ImGui::ShowDemoWindow(&show_demo_windows);
            ImPlot::ShowDemoWindow(&show_demo_windows);
        }

        if (show_simulation_window)
        {
            ImGui::Begin("Simulation");
            ImGui::SeparatorText("Parameters");
            ImGui::InputScalar("G Start", ImGuiDataType_Float, &sim_g_start, NULL);
            ImGui::InputScalar("G Stop", ImGuiDataType_Float, &sim_g_stop, NULL);
            ImGui::InputScalar("T Start", ImGuiDataType_Float, &sim_t_start, NULL);
            ImGui::InputScalar("T Stop", ImGuiDataType_Float, &sim_t_stop, NULL);
            ImGui::InputScalar("Total time", ImGuiDataType_Float, &sim_time_s, NULL);
            ImGui::InputScalar("Steps", ImGuiDataType_S32, &sim_steps, NULL);

            ImGui::Separator();

            if (ImGui::Button("Start"))
            {
                if (!simulator.thread_active)
                {
                    std::thread sim_t(
                        &PV::Simulator::Simulation,
                        &simulator,
                        sim_g_start,
                        sim_g_stop,
                        sim_t_start,
                        sim_t_stop,
                        sim_time_s,
                        sim_steps
                    );
                    sim_t.detach();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Stop")) simulator.enable_simulation = false;
            ImGui::ProgressBar(sim_progress, ImVec2(0.0f, 0.0f));
            ImGui::End();
        }

        if (show_parameter_window)
        {
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
            ImGui::Checkbox("Show real-time pairs", &show_real_time_pairs);
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

            // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        if (show_current_voltage_plot_window)
        {
            ImGui::Begin("Current - Voltage Plot");

            if (ImPlot::BeginPlot("I-V Plot", ImVec2(-1, -1)))
            {
                ImPlot::SetupAxes("Voltage (V)", "Current (A)");
                ImPlot::SetupAxesLimits(0, 1.1 * v_oc, 0, 1.1 * i_sc);

                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.20f);

                ImPlot::PlotShaded("I-V plot", pvModule.GetVoltageArray(), pvModule.GetCurrentArray(), prev_voltage_steps);
                ImPlot::PlotLine("I-V plot", pvModule.GetVoltageArray(), pvModule.GetCurrentArray(), prev_voltage_steps);

                // Show real time
                if (show_real_time_pairs) ImPlot::PlotScatter("Real Time (IV)", rt_v, rt_i, 1);

                if (show_nominal_curves)
                {
                    ImPlot::PlotShaded("I-V plot Nominal", pvModuleNominal.GetVoltageArray(), pvModuleNominal.GetCurrentArray(), PV::STEPS_nominal);
                    ImPlot::PlotLine("I-V plot Nominal", pvModuleNominal.GetVoltageArray(), pvModuleNominal.GetCurrentArray(), PV::STEPS_nominal);
                }

                ImPlot::EndPlot();
            }
            ImGui::End();
        }

        if (show_power_voltage_plot_window)
        {
            ImGui::Begin("Power - Voltage Plot");
            if (ImPlot::BeginPlot("P-V Plot", ImVec2(-1, -1)))
            {
                ImPlot::SetupAxes("Voltage (V)", "Power (W)");
                ImPlot::SetupAxesLimits(0, 1.1 * v_oc, 0, 1.1 * i_sc * v_oc);

                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.20f);

                ImPlot::PlotShaded("P-V plot", pvModule.GetVoltageArray(), pvModule.GetPowerArray(), prev_voltage_steps);
                ImPlot::PlotLine("P-V plot", pvModule.GetVoltageArray(), pvModule.GetPowerArray(), prev_voltage_steps);

                // Show real time
                if (show_real_time_pairs) ImPlot::PlotScatter("Real Time (PV)", rt_v, rt_p, 1);

                if (show_nominal_curves)
                {
                    ImPlot::PlotShaded("P-V plot Nominal", pvModuleNominal.GetVoltageArray(), pvModuleNominal.GetPowerArray(), PV::STEPS_nominal);
                    ImPlot::PlotLine("P-V plot Nominal", pvModuleNominal.GetVoltageArray(), pvModuleNominal.GetPowerArray(), PV::STEPS_nominal);
                }

                ImPlot::EndPlot();
            }
            ImGui::End();
        }
    }

private:
    bool show_demo_windows = false;
    bool show_simulation_window = true;
    bool show_parameter_window = true;
    bool show_current_voltage_plot_window = true;
    bool show_power_voltage_plot_window = true;
};

int main(int, char**)
{
    PVWatchApp app;
    app.Run();
    return 0;
}
