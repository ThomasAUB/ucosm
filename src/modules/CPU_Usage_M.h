#pragma once

#include "ucosm-sys-data.h"

#include <limits>


struct CPU_Usage_M
{

	fine_tick_t getCPU_UsagePercent(){
		if(mLoopTime == 0){ return 100; }
		return ( ( mExeTime * 100 )/ mLoopTime );
	}

	fine_tick_t getExecutionTime(){
		return mExeTime;
	}

	fine_tick_t getMaxExecutionTime(){
		return mMaxExeTime;
	}

	fine_tick_t getCallPeriod(){
		return mLoopTime;
	}

	template<typename T>
	void init(T *t) {
		mStartExeTS = 0;
		mExeTime = 0;
		mMaxExeTime = 0;
		mStartLoopTS = 0;
		mLoopTime = 1;
	}

	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 

	template<typename T>
	void makePreExe(T *t){
		mStartExeTS = SysKernelData::sGetFineTick();
	}

	template<typename T>
	void makePostExe(T *t){
		fine_tick_t curTS = SysKernelData::sGetFineTick();

		mExeTime = curTS - mStartExeTS;

		if(mExeTime > mMaxExeTime){
			mMaxExeTime = mExeTime;
		}

		mLoopTime = curTS - mStartLoopTS;
		mStartLoopTS = curTS;
	}

	template<typename T>
	void makePreDel(T *t){}
	
private:

	fine_tick_t mStartExeTS;
	fine_tick_t mExeTime;
	fine_tick_t mMaxExeTime;

	fine_tick_t mStartLoopTS;
	fine_tick_t mLoopTime;

};

