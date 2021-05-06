#pragma once

#include "ucosm-sys-data.h"

struct Priority_M {

    void setPriority(const uint8_t inPrio) {
        // priority can't be inferior to 1
        mPriority = (inPrio) ? inPrio : 1;
    }

    void init() {
        mPriority = 1;
    }

    bool isExeReady() const {
        if (SysKernelData::sCnt) {
            return (!(SysKernelData::sCnt % mPriority));
        } else {
            return (!((SysKernelData::sCnt + 1) % mPriority));
        }
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

    uint8_t mPriority;
};

