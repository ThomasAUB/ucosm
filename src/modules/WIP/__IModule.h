#pragma once


struct IModule
{

	virtual void init(){}

    	virtual bool isExeReady() const { return true; }

	virtual bool isDelReady() const { return true; }

	virtual void makePreExe(){}

	virtual void makePostExe(){}

	virtual void makePreDel(){}

};
