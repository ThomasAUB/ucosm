#pragma once

#include "ucosm-sys-data.h"

struct Delay_M {

    void setDelay(tick_t inDelay) {
        mExecution_time_stamp = SysKernelData::sGetTick() + inDelay;
    }

    tick_t getDelay() {
        if (mExecution_time_stamp > SysKernelData::sGetTick()) {
            return mExecution_time_stamp - SysKernelData::sGetTick();
        } else {
            return 0;
        }
    }

    void init() {
        mExecution_time_stamp = SysKernelData::sGetTick();
    }

    bool isExeReady() const {
        return (SysKernelData::sGetTick() >= mExecution_time_stamp);
    }

    bool isDelReady() const {
        return true;
    }

    void makePreExe() {
    }

    void makePreDel() {
    }

    void makePostExe() {
    }

private:

    tick_t mExecution_time_stamp;
};

