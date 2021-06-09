#pragma once

#include "../ucosm_sys_data.h"

// allow to specify a time period of execution of the process or postpone 
// its execution
struct Interval_M {

    using period_t = uint16_t;

    void setPeriod(period_t inPeriod) {
        mPeriod = inPeriod;
    }

    void setDelay(UcosmSysData::tick_t inDelay) {
        mExecution_time_stamp = UcosmSysData::sGetTick() + inDelay;
    }

    period_t getPeriod() {
        return mPeriod;
    }

    UcosmSysData::tick_t getDelay() {
        if (mExecution_time_stamp > UcosmSysData::sGetTick()) {
            return mExecution_time_stamp - UcosmSysData::sGetTick();
        } else {
            return 0;
        }
    }

    void init() {
        mPeriod = 0;
        mExecution_time_stamp = UcosmSysData::sGetTick();
    }

    bool isExeReady() const {
        return (UcosmSysData::sGetTick() >= mExecution_time_stamp);
    }

    bool isDelReady() const {
        return true;
    }

    void makePreExe() {
        mExecution_time_stamp += mPeriod;
    }

    void makePreDel() {
    }

    void makePostExe() {
    }

private:

    UcosmSysData::tick_t mExecution_time_stamp;
    period_t mPeriod;
};

