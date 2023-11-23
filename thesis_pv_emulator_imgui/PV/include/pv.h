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
	class PVModule
	{
	public:
		/*
			Calculate current array using analytical method.
			Inputs: Voc (V), Isc (A), Vmp (V), Isc (A), The irradiance G in W/m2, and the cell temperature
			Output: void (Writes the current data to the this.current_array [float array]
		*/
		void CalculateCurrentArray(float v_oc, float i_sc, float v_mp, float i_mp, float g, float t_e, int steps, int iterations);

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

	
	private:
		// PV internal parameters

		double Voc;
		double Isc;
		double Vmp;
		double Imp;

		double G;
		double T;

		double Vthermal;
		double Rsh;
		double Rs;
		double I0;
		double a;
		double Ns;
		double Np;
		double idealityFactor;

		float Ipv;

		// Calculation parameters
		int steps;
		int iters;

		// Array pointers
		double* current_array;
		double* voltage_array;
		double* power_array;
	};

}