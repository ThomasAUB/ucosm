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

    struct TaskletBackend;

    template<interrupt_id_t interrupt_count, const TaskletBackend& backend>
    struct TaskletScheduler;

    struct ITasklet : ITask<priority_t> {

        void setPriority(priority_t inPriority) {
            mPriority = inPriority;
        }

        priority_t getPriority() const {
            return mPriority;
        }

        void setPeriod(tick_t inPeriod) {
            mState = eState::sleeping;
            mPeriod = inPeriod > 0 ? inPeriod : 1;
            mInterruptID = invalid_interrupt_id;
        }

        tick_t getPeriod() const {
            return mPeriod;
        }

        void waitForInterrupt(interrupt_id_t inInterruptID) {
            mInterruptID = inInterruptID;
            mState = eState::waitingForInterrupt;
            mPeriod = 0;
        }

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

        bool isConfigured() const {
            return mState != eState::unconfigured;
        }

        interrupt_id_t getInterruptID() const {
            return mInterruptID;
        }

        ~ITasklet() = default;

    protected:

        // Demoted from ITask's public removeTask() : use
        // TaskletScheduler::removeTask() from outside, or this->removeTask()
        // from within run().
        using ITask<priority_t>::removeTask;

    private:

        template<interrupt_id_t, const TaskletBackend&>
        friend struct TaskletScheduler;

        void setScheduledDeadline(tick_t inDeadline) {
            mScheduledDeadline = inDeadline;
        }

        tick_t getScheduledDeadline() const {
            return mScheduledDeadline;
        }

        enum class eState : uint8_t {
            unconfigured,
            sleeping,
            waitingForInterrupt
        };

        static constexpr interrupt_id_t invalid_interrupt_id = static_cast<interrupt_id_t>(-1);

        priority_t mPriority = static_cast<priority_t>(-1);
        tick_t mPeriod = 0;
        tick_t mScheduledDeadline = 0;
        interrupt_id_t mInterruptID = invalid_interrupt_id;
        eState mState = eState::unconfigured;
    };

}