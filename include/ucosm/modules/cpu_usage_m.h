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

#include "../ucosm_sys_data.h"

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
        mStartExeTS = UcosmSysData::sGetFineTick();
    }

    void makePostExe() {
        fine_tick_t curTS = UcosmSysData::sGetFineTick();

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

