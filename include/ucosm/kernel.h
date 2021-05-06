/*
 * Copyright (C) 2020 Thomas AUBERT <aubert.thms@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Thomas AUBERT'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * uCosmDev IS PROVIDED BY Thomas AUBERT ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Thomas AUBERT OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <cstddef>

#include "IScheduler.h"

#include "void_M.h"

// sCnt
#include "modules/ucosm-sys-data.h"
uint8_t SysKernelData::sCnt = 0;

template<size_t max_task_count, typename module_M = void_M>
class Kernel: public IScheduler {

    using task_index_t = uint8_t;

public:

    Kernel() :
            mTaskCount(0), mIdleTask(nullptr) {
    }

    template<typename T>
    bool addTask(T *inTask) {
        if (mTaskCount == max_task_count) {
            return false;
        }
        mTasks[mTaskCount] = static_cast<IScheduler*>(inTask);
        mTaskTraits[mTaskCount].init();
        mTaskCount++;
        return true;
    }

    module_M* getTask(IScheduler *inTask) {
        task_index_t i;
        if (getTaskIndex(inTask, i)) {
            return &mTaskTraits[i];
        }
        return nullptr;
    }

    bool removeTask(IScheduler *inTask) {
        task_index_t i;
        if (getTaskIndex(inTask, i)) {

            if (!mTaskTraits[i].isDelReady()) {
                return false;
            }

            mTaskTraits[i].makePreDel();

            // shift tasks for contiguous array
            while (i < mTaskCount - 1) {
                mTasks[i] = mTasks[i + 1];
                mTaskTraits[i] = mTaskTraits[i + 1];
                i++;
            }
            mTaskCount--;
        }
        return true;
    }

    bool schedule() final {

        bool hasExe = false;

        task_index_t i = 0;

        SysKernelData::sCnt++;

        while (i < mTaskCount) {

            if (mTasks[i] && mTaskTraits[i].isExeReady()) {
                mTaskTraits[i].makePreExe();
                hasExe |= mTasks[i]->schedule();
                mTaskTraits[i].makePostExe();
            }
            i++;
        }

        if (!hasExe && mIdleTask) {
            // idle task if exists
            mIdleTask();
        }

        return hasExe;
    }

    void setIdleTask(void (*inIdleTask)()) {
        mIdleTask = inIdleTask;
    }

private:

    bool getTaskIndex(IScheduler *inScheduler, task_index_t &ioIndex) {
        if (!mTaskCount) {
            return false;
        }
        ioIndex = 0;
        do {
            if (mTasks[ioIndex] == inScheduler) {
                return true;
            }
        } while (++ioIndex < mTaskCount);
        return false;
    }

    IScheduler *mTasks[max_task_count];

    module_M mTaskTraits[max_task_count];

    task_index_t mTaskCount;

    void (*mIdleTask)();

};

