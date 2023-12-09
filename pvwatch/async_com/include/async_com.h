#pragma once

class AsyncCommunication
{
public:
	AsyncCommunication();
	~AsyncCommunication();
	/*
		A method to test Async write
	*/
	void Test(void);

	/*
		Get data from a COM port, TODO
	*/
	void GetDatafromCOMPort(void);

};