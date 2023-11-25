#include <iostream>
#include <random>
#include <math.h>
#include <thread>
#include <chrono>

#include "../include/pv.h"


void PV::PVModule::ClearCurrentArray()
{
	delete[] this->current_array;
	delete[] this->voltage_array;
	delete[] this->power_array;

	this->current_array = new double[0];
	this->voltage_array = new double[0];
	this->power_array = new double[0];
}

void PV::PVModule::CalculateIVPArrays(float v_oc,float i_sc, float v_mp, float i_mp, float g, float t_e, int steps, int iterations)
{
	// Set up calculation parameters
	this->steps = steps >= 0 ? steps : 0;
	this->iters = iterations >= 0 ? iterations : 0;

	// Set up current, voltage, and power arrays
	this->current_array = new double[this->steps];
	this->voltage_array = new double[this->steps];
	this->power_array	= new double[this->steps];

	// Setup nominal parameters
	this->Voc_nom = (double)v_oc;
	this->Isc_nom = (double)i_sc;
	this->Vmp_nom = (double)v_mp;
	this->Imp_nom = (double)i_mp;
	this->G_nom = 1000;
	this->T_nom = 25;

	this->Vthermal = k * (this->T + 274.15) / q;

	// Set up intrinsic PV parameters
	this->G = (double)g;
	this->T = (double)t_e;
	this->Voc = this->Voc_nom + (this->Vthermal * log(this->G / this->G_nom));
	this->Isc = (double)i_sc;
	this->Vmp = (double)v_mp;
	this->Imp = (double)i_mp;

	//Initialize this values for convergence
	//According to https://oa.upm.es/30693/1/2014ICREARA.pdf

	this->Rsh = 100;
	this->Rs = 1;
	this->I0 = 0;
	this->a = 1;
	this->Ns = 1;
	this->Np = 1;
	this->idealityFactor = 1;
	this->Ipv = 0;

	//Calculate a including Number of series and parallel cells
	this->Ns = this->Voc_nom / UNITY_CELL_VOC;
	if (this->Ns - floor(this->Ns) > 0.5) this->Ns = floor(this->Ns) + 1;
	else this->Ns = floor(this->Ns);

	this->Np = this->Isc / UNITY_CELL_ISC;
	if (this->Np - floor(this->Np) > 0.5) this->Np = floor(this->Np) + 1;
	else this->Np = floor(this->Np);

	this->a = this->Ns / this->Np;
	this->a *= this->idealityFactor;


	//Calculate Rs based on the above mentioned paper
	for (int i = 0; i < this->iters; i++)
	{
		double eq_10_num = this->a * this->Vthermal * this->Vmp * (2 * this->Imp - this->Isc);
		double eq_10_den = (this->Vmp * this->Isc + this->Voc_nom * (this->Imp - this->Isc)) * 
			(this->Vmp - this->Imp * this->Rs) - this->a * this->Vthermal * 
			(this->Vmp * this->Isc - this->Voc_nom * this->Imp);
		
		this->Rs = (this->a * this->Vthermal * log(eq_10_num / eq_10_den) + this->Voc_nom - this->Vmp) / this->Imp;
	}

	//Calculate Rsh based on the above mentioned paper
	float eq_11_num = (this->Vmp * this->Imp * this->Rs) * (this->Vmp - this->Rs * (this->Isc - this->Imp) - this->a * this->Vthermal);
	float eq_11_den = (this->Vmp - this->Imp * this->Rs) * (this->Isc - this->Imp) - this->a * this->Vthermal * this->Imp;

	this->Rsh = eq_11_num / eq_11_den;

	//calculate I0
	float eq_6_num = (this->Rsh + this->Rs) * this->Isc - this->Voc_nom;
	float eq_6_den = this->Rsh * exp(this->Voc_nom / (this->a * this->Vthermal));

	this->I0 = eq_6_num / eq_6_den;

	//Calculate photocurrent Ipv
	
	this->Ipv_nom = ((this->Rsh + this->Rs) / this->Rsh) * this->Isc_nom;
	this->Ipv = (this->G / this->G_nom) * this->Ipv_nom;

	for (int i = 0; i < this->steps; i++)
	{
		//Fill V array and zero the I array
		this->voltage_array[i] = (double)i * this->Voc / (double)(this->steps - 1);
		this->current_array[i] = 0;

		for (int j = 0; j < this->iters; j++)
		{
			double voltageAtThisPoint = (double)i * this->Voc / (double)(this->steps - 1);
			//float voltageAtThisPoint = this->voltageLookUpTable[i];
			double exponent_value = (voltageAtThisPoint + this->current_array[i] * this->Rs) / (this->a * this->Vthermal);
			double term1 = this->I0 * (exp(exponent_value) - 1);
			double term2 = (voltageAtThisPoint + this->current_array[i] * this->Rs) / this->Rsh;
			this->current_array[i] = this->Ipv - term1 - term2;
		}
		
		// Clip current to avoid negative values
		if (this->current_array[i] < 0) this->current_array[i] = 0;
		
		// Fill the power array
		this->power_array[i] = this->voltage_array[i] * this->current_array[i];
	}
}

double* PV::PVModule::GetCurrentArray()
{
	return this->current_array;
}

double* PV::PVModule::GetVoltageArray()
{
	return this->voltage_array;
}

double* PV::PVModule::GetPowerArray()
{
	return this->power_array;
}

double PV::PVModule::GetCurrentFromVoltage(double voltage)
{
	if (this->steps <= 1) return this->Isc;

	double real_voltage = (voltage > this->Voc) ? this->Voc : voltage;

	double approx_voltage_idx = real_voltage / (this->Voc / (this->steps - 1));

	int voltage_floor_idx = (int)floor(approx_voltage_idx);
	int voltage_ceil_idx = (int)ceil(approx_voltage_idx);

	double voltage_floor = this->voltage_array[voltage_floor_idx];
	double voltage_ceil = this->voltage_array[voltage_ceil_idx];

	double current_floor = this->current_array[voltage_floor_idx];
	double current_ceil = this->current_array[voltage_ceil_idx];

	// Calculate semi linear current through the slope of the floor and ceil currents
	double slope = (current_ceil - current_floor) / (voltage_ceil - voltage_floor);
	
	double current = ((real_voltage - voltage_floor) * slope) + current_floor;

	return current;
}


void PV::Simulator::Simulation(double G_start, double G_stop, double T_start, double T_stop, double time_secs, double sim_steps)
{
	// EXEI THEMA TO FIX LATER
	this->G_start = G_start;
	this->G_stop = G_stop;
	this->T_start = T_start;
	this->T_stop = T_stop;
	this->time_secs = time_secs;
	this->sim_steps = sim_steps;

	// Get the current displayed pv module
	extern PVModule pvModule;

	std::cout << "Simulation started" << std::endl;

	double G_step = abs(this->G_stop - this->G_start) / this->sim_steps;
	double T_step = abs(this->T_stop - this->T_start) / this->sim_steps;

	int time_in_millis = (int)floor((1000 * this->time_secs) / this->sim_steps);

	for (int i = 0; i < this->sim_steps; i++)
	{
		pvModule.CalculateIVPArrays(
			pvModule.Voc,
			pvModule.Isc,
			pvModule.Vmp,
			pvModule.Imp,
			this->G_start + G_step * (double)i,
			this->T_start + T_step * (double)i,
			pvModule.steps,
			pvModule.iters
		);
		
		std::this_thread::sleep_for(std::chrono::microseconds(time_in_millis));
	}

	return;
}