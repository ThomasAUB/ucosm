/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2020 Thomas AUBERT                                                *
 *                                                                                 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy    *
 * of this software and associated documentation files (the "Software"), to deal   *
 * in the Software without restriction, including without limitation the rights    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is           *
 * furnished to do so, subject to the following conditions:                        *
 *                                                                                 *
 * The above copyright notice and this permission notice shall be included in all  *
 * copies or substantial portions of the Software.                                 *
 *                                                                                 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 * SOFTWARE.                                                                       *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include "ucosm/ucosm_sys_data.h"

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
        mExecution_time_stamp = 0;
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

