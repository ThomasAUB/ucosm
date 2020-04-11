#pragma once

#include "ucosm-sys-data.h"

// periodic call of the function, guarantees a constant average execution rate
struct Periodic_M // 6 bytes
{

	using period_t = uint16_t;

	
	void setPeriod(period_t inPeriod)
	{
		mPeriod = inPeriod;
		mExecution_time_stamp += mPeriod;
	}

	period_t getPeriod()
	{
		return mPeriod;
	}

	void setDelay(tick_t inDelay)
	{	
		mExecution_time_stamp = SysKernelData::sGetTick()+inDelay; 
	}
	
	tick_t getDelay()
	{	if(mExecution_time_stamp > SysKernelData::sGetTick()){
			return mExecution_time_stamp - SysKernelData::sGetTick(); 
		}else{
			return 0;
		}
	}

	template<typename T>
	void init(T *t)
	{
		mExecution_time_stamp = SysKernelData::sGetTick();
	}
	template<typename T>
	bool isExeReady(T *t) const {
		return (SysKernelData::sGetTick() >= mExecution_time_stamp);
	}
	template<typename T>
	bool isDelReady(T *t) { return true; }
	template<typename T>
	void makePreExe(T *t)
	{
		mExecution_time_stamp += mPeriod;
	}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}
		
private:
	
	tick_t mExecution_time_stamp;
	period_t mPeriod;
};

