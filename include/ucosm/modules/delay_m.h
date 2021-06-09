#pragma once

#include "../ucosm_sys_data.h"

struct Delay_M {

    void setDelay(UcosmSysData::tick_t inDelay) {
        mExecution_time_stamp = UcosmSysData::sGetTick() + inDelay;
    }

    UcosmSysData::tick_t getDelay() {
        if (mExecution_time_stamp > UcosmSysData::sGetTick()) {
            return mExecution_time_stamp - UcosmSysData::sGetTick();
        } else {
            return 0;
        }
    }

    void init() {
        mExecution_time_stamp = UcosmSysData::sGetTick();
    }

    bool isExeReady() const {
        return (UcosmSysData::sGetTick() >= mExecution_time_stamp);
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

    UcosmSysData::tick_t mExecution_time_stamp;
};

