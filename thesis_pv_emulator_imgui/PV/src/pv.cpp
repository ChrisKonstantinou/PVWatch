#include <iostream>
#include <random>
#include <math.h>
#include "../include/pv.h"


void PV::PVModule::ClearCurrentArray()
{
	for (int i = 0; i < NUMBER_OF_POINTS; i++)
	{
		this->current_array[i] = 0.0;
		this->voltage_array[i] = 0.0;
	}
}

void PV::PVModule::CalculateCurrentArray(float v_oc,float i_sc, float v_mp, float i_mp, float g, float t_e)
{
	this->Voc	= (double)v_oc;
	this->Isc	= (double)i_sc;
	this->Vmp	= (double)v_mp;
	this->Imp	= (double)i_mp;
	this->G		= (double)g;
	this->T		= (double)t_e;

	//Initialize this values for convergence
	//According to https://oa.upm.es/30693/1/2014ICREARA.pdf

	this->Vthermal = k * (this->T + 274.15) / q;

	this->Rsh = 100;
	this->Rs = 1;
	this->I0 = 0;
	this->a = 1;
	this->Ns = 1;
	this->Np = 1;
	this->idealityFactor = 1;
	this->Ipv = 0;

	//Calculate a including Number of series and parallel cells
	this->Ns = this->Voc / UNITY_CELL_VOC;
	if (this->Ns - floor(this->Ns) > 0.5) this->Ns = floor(this->Ns) + 1;
	else this->Ns = floor(this->Ns);

	this->Np = this->Isc / UNITY_CELL_ISC;
	if (this->Np - floor(this->Np) > 0.5) this->Np = floor(this->Np) + 1;
	else this->Np = floor(this->Np);

	this->a = this->Ns / this->Np;
	this->a *= this->idealityFactor;


	//Calculate Rs based on the above mentioned paper
	for (int i = 0; i < MAX_ITERATIONS; i++)
	{
		double eq_10_num = this->a * this->Vthermal * this->Vmp * (2 * this->Imp - this->Isc);
		double eq_10_den = (this->Vmp * this->Isc + this->Voc * (this->Imp - this->Isc)) * 
			(this->Vmp - this->Imp * this->Rs) - this->a * this->Vthermal * 
			(this->Vmp * this->Isc - this->Voc * this->Imp);
		
		this->Rs = (this->a * this->Vthermal * log(eq_10_num / eq_10_den) + this->Voc - this->Vmp) / this->Imp;
	}

	//Calculate Rsh based on the above mentioned paper
	float eq_11_num = (this->Vmp * this->Imp * this->Rs) * (this->Vmp - this->Rs * (this->Isc - this->Imp) - this->a * this->Vthermal);
	float eq_11_den = (this->Vmp - this->Imp * this->Rs) * (this->Isc - this->Imp) - this->a * this->Vthermal * this->Imp;

	this->Rsh = eq_11_num / eq_11_den;

	//calculate I0
	float eq_6_num = (this->Rsh + this->Rs) * this->Isc - this->Voc;
	float eq_6_den = this->Rsh * exp(this->Voc / (this->a * this->Vthermal));

	this->I0 = eq_6_num / eq_6_den;

	//Calculate photocurrent Ipv
	//To Ipv exei eksarthsh kai apo to G kai apo thn gwnia prosptvshs **NEEDS FIXING**
	this->Ipv = ((this->Rsh + this->Rs) / this->Rsh) * this->Isc;

	//Fill V array and zero the I array
	for (int i = 0; i < NUMBER_OF_POINTS; i++)
	{
		this->voltage_array[i] = (double)i*this->Voc/(double)(NUMBER_OF_POINTS - 1);
		this->current_array[i] = 0;
	}

	for (int i = 0; i < NUMBER_OF_POINTS; i++)
	{
		for (int j = 0; j < MAX_ITERATIONS; j++)
		{
			double voltageAtThisPoint = (double)i * this->Voc / (double)(NUMBER_OF_POINTS - 1);
			//float voltageAtThisPoint = this->voltageLookUpTable[i];
			double exponent_value = (voltageAtThisPoint + this->current_array[i] * this->Rs) / (this->a * this->Vthermal);
			double term1 = this->I0 * (exp(exponent_value) - 1);
			double term2 = (voltageAtThisPoint + this->current_array[i] * this->Rs) / this->Rsh;
			this->current_array[i] = this->Ipv - term1 - term2;
		}
	}
}