#include "../include/async_com.h"
#include "../../pv/include/pv.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

// extern std::mutex mtx;

extern double rt_v[1];
extern double rt_i[1];
extern double rt_p[1];

// Test if we have acess to PV class
extern PV::PVModule pvModule; // main PV module handle

AsyncCommunication::AsyncCommunication()
{
	// std::cout << "AsyncCommunication Initialized" << std::endl;
}

AsyncCommunication::~AsyncCommunication()
{
	// std::cout << "AsyncCommunication Destroyed" << std::endl;
}


void AsyncCommunication::Test()
{
	while (true)
	{
		for (int i = 0; i < 35 * 4; i++)
		{
			// std::lock_guard<std::mutex> lock(mtx); // FIX MUTEX ISSUE LATER
			
			rt_v[0] = (double)i / 4.0;
			
			rt_i[0] = pvModule.GetCurrentFromVoltage(rt_v[0]);

			rt_p[0] = rt_v[0] * rt_i[0];

			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
}

void AsyncCommunication::GetDatafromCOMPort()
{

}
