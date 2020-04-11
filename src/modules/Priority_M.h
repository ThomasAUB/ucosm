#pragma once


// get tick
#include "ucosm-sys-data.h"

struct Priority_M
{

	void setPriority(const uint8_t inPrio)
	{
		// priority can't be inferior to 1
		mPriority = (inPrio)?inPrio:1; 
	}

	template<typename T>
	void init(T *t)
	{
		mPriority = 1;	
	}
	
	template<typename T>
    bool isExeReady(T *t) const 
	{
		if(SysKernelData::sCnt)
		{
			return (!(SysKernelData::sCnt%mPriority));
		}else{
			return (!((SysKernelData::sCnt+1)%mPriority));
		}
	}
	template<typename T>
	bool isDelReady(T *t) const {return true;}
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}

private:

	uint8_t mPriority;
};


