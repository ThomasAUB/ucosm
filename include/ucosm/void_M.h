#pragma once


struct void_M
{

	void init(){}

	bool isExeReady() const {return true;}

	bool isDelReady() const {return true;}

	void makePreExe(){}

	void makePreDel(){}

	void makePostExe(){}
};
