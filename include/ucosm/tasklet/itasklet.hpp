/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2026 Thomas AUBERT                                                *
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
 *                                                                                 *
 * github : https://github.com/ThomasAUB/ucosm                                     *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include <stdint.h>
#include "ucosm/core/itask.hpp"

namespace ucosm {

    using tick_t = uint32_t;
    using priority_t = tick_t;
    using interrupt_id_t = uint8_t;

    struct ITasklet : ITask<priority_t> {

        void setPriority(priority_t inPriority) {
            mPriority = inPriority;
        }

        priority_t getPriority() const {
            return mPriority;
        }

        // Schedules the task on the timer list and sets how often it runs.
        // The period is kept after the task has run : a task that leaves its
        // state untouched in run() is re-armed with the same value.
        // A period of 0 means "as soon as possible", which for a tasklet is
        // the next tick : re-arming a task on the tick it just ran at would
        // keep the low priority handler from ever returning.
        // The delay before the next execution is held by the task rank, like
        // IPeriodicTask : see TaskletScheduler::addTask and setDelay.
        void setPeriod(tick_t inPeriod) {
            mState = eState::sleeping;
            mPeriod = inPeriod;
            mInterruptID = invalid_interrupt_id;
        }

        tick_t getPeriod() const {
            return mPeriod;
        }

        // Subscribes the task to an interrupt. The subscription is kept after
        // the task has run, symmetrically with setPeriod().
        void waitForInterrupt(interrupt_id_t inInterruptID) {
            mInterruptID = inInterruptID;
            mState = eState::waitingForInterrupt;
            mPeriod = 0;
        }

        // Clears the task configuration : it goes back to the state it had
        // before the first setPeriod() / waitForInterrupt(). A task that
        // disposes itself from run() is unlinked by the scheduler, and a
        // disposed task is refused by addTask() until it is configured again.
        void dispose() {
            mState = eState::unconfigured;
            mInterruptID = invalid_interrupt_id;
            mPeriod = 0;
        }

        bool isSleeping() const {
            return mState == eState::sleeping;
        }

        bool isWaitingForInterrupt() const {
            return mState == eState::waitingForInterrupt;
        }

        // Tells whether the task went through setPeriod() or
        // waitForInterrupt() and can therefore be scheduled.
        bool isConfigured() const {
            return mState != eState::unconfigured;
        }

        interrupt_id_t getInterruptID() const {
            return mInterruptID;
        }

        ~ITasklet() = default;

    private:

        enum class eState : uint8_t {
            unconfigured,
            sleeping,
            waitingForInterrupt
        };

        static constexpr interrupt_id_t invalid_interrupt_id = static_cast<interrupt_id_t>(-1);

        priority_t mPriority = static_cast<priority_t>(-1);
        tick_t mPeriod = 0;
        interrupt_id_t mInterruptID = invalid_interrupt_id;
        eState mState = eState::unconfigured;
    };

}