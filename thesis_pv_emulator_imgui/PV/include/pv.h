#pragma once
#include <string>
#include <vector>

#define k 1.38064852e-23
#define q 1.602176634e-19

#define RANGE_FACTOR 1.25
#define UNITY_CELL_VOC 0.7
#define UNITY_CELL_ISC 8.5

namespace PV
{
	static float G_nominal = 1000.0;
	static float T_nominal = 25.0;
	static int STEPS_nominal = 200;
	static int ITERS_nominal = 50;

	class PVModule
	{
	public:
		// PV internal parameters
		double Voc;
		double Isc;
		double Vmp;
		double Imp;

		double G;
		double T;

		// Calculation parameters
		int steps;
		int iters;

		/*
			Calculate I, V, P arrays using analytical method.
			Inputs: Voc (V), Isc (A), Vmp (V), Isc (A), The irradiance G in W/m2, and the cell temperature
			Output: void (Writes the current data to the this.current_array [float array]
		*/
		void CalculateIVPArrays(float v_oc, float i_sc, float v_mp, float i_mp, float g, float t_e, int steps, int iterations);

		/*
			Clears the current array
		*/
		void ClearCurrentArray(void);

		/*
			Get the Current Array
		*/
		double* GetCurrentArray();

		/*
			Get the Voltage Array
		*/
		double* GetVoltageArray();
		
		/*
			Get the Power Array
		*/
		double* GetPowerArray();

		/*
			Get current value from the voltage, Implements also a linear approximation between
			two closest values.
		*/
		double GetCurrentFromVoltage(double voltage);
	
	private:

		// Nominal Parameters
		double Voc_nom;
		double Isc_nom;
		double Vmp_nom;
		double Imp_nom;

		double G_nom;
		double T_nom;

		// ----- //
		double Vthermal;
		double Rsh;
		double Rs;
		double I0;
		double a;
		double Ns;
		double Np;
		double idealityFactor;

		double Ipv;
		double Ipv_nom;

		// Array pointers
		double* current_array;
		double* voltage_array;
		double* power_array;
	};

	class Simulator
	{
	public:
		// Maybe add here the extern handle to the current PV module
		// And add the extern handle of the sim_progress

		/*
			Start a simulation sweeping values for G and T from G_start to G_stop, T_start and T_stop
			in a set time (seconds) time_secs
		*/
		void Simulation(double G_start, double G_stop, double T_start, double T_stop, double time_secs, double sim_steps);
	
	private:
		double G_start;
		double G_stop;
		double T_start;
		double T_stop;
		double time_secs;
		double sim_steps;
	};
}