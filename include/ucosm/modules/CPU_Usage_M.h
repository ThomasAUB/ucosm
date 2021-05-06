#pragma once

#include "ucosm-sys-data.h"

#include <limits>

// provides a debug tool to measure the cpu usage of the process associated with
// this module

struct CPU_Usage_M {

    fine_tick_t getCPU_UsagePercent() {
        if (mLoopTime == 0) {
            return 100;
        }
        return ((mExeTime * 100) / mLoopTime);
    }

    fine_tick_t getExecutionTime() {
        return mExeTime;
    }

    fine_tick_t getMaxExecutionTime() {
        return mMaxExeTime;
    }

    fine_tick_t getCallPeriod() {
        return mLoopTime;
    }

    void init() {
        mStartExeTS = 0;
        mExeTime = 0;
        mMaxExeTime = 0;
        mStartLoopTS = 0;
        mLoopTime = 1;
    }

    bool isExeReady() const {
        return true;
    }

    bool isDelReady() const {
        return true;
    }

    void makePreExe() {
        mStartExeTS = SysKernelData::sGetFineTick();
    }

    void makePostExe() {
        fine_tick_t curTS = SysKernelData::sGetFineTick();

        mExeTime = curTS - mStartExeTS;

        if (mExeTime > mMaxExeTime) {
            mMaxExeTime = mExeTime;
        }

        mLoopTime = curTS - mStartLoopTS;
        mStartLoopTS = curTS;
    }

    void makePreDel() {
    }

private:

    fine_tick_t mStartExeTS;
    fine_tick_t mExeTime;
    fine_tick_t mMaxExeTime;

    fine_tick_t mStartLoopTS;
    fine_tick_t mLoopTime;

};

