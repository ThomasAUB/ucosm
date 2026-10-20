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

    struct TaskletScheduler;
    struct TaskletEvent;

    struct ITasklet : ITask<priority_t> {

        void setPriority(priority_t inPriority) {
            mPriority = inPriority;
        }

        priority_t getPriority() const {
            return mPriority;
        }

        void setPeriod(tick_t inPeriod) {
            mState = eState::waitingForTimer;
            mPeriod = inPeriod > 0 ? inPeriod : 1;
        }

        // 0 when the task is not waiting for the timer.
        tick_t getPeriod() const {
            return isWaitingForTimer() ? mPeriod : 0;
        }

        // The event must outlive the wait. Waiting doesn't modify its
        // observable state, hence the const reference : an event owner can
        // hand out a wait-only view of it.
        void waitForEvent(const TaskletEvent& inEvent) {
            mState = eState::waitingForEvent;
            mEvent = &inEvent;
        }

        void dispose() {
            mState = eState::unconfigured;
            mPeriod = 0;
        }

        bool isWaitingForTimer() const {
            return mState == eState::waitingForTimer;
        }

        bool isWaitingForEvent() const {
            return mState == eState::waitingForEvent;
        }

        bool isConfigured() const {
            return mState != eState::unconfigured;
        }

        // nullptr when the task is not waiting for an event.
        const TaskletEvent* getEvent() const {
            return isWaitingForEvent() ? mEvent : nullptr;
        }

        ~ITasklet() = default;

    protected:

        // Use TaskletScheduler::removeTask() from outside the task's run().
        using ITask<priority_t>::removeTask;

    private:

        friend struct TaskletScheduler;

        void setScheduledDeadline(tick_t inDeadline) {
            mScheduledDeadline = inDeadline;
        }

        tick_t getScheduledDeadline() const {
            return mScheduledDeadline;
        }

        enum class eState : uint8_t {
            unconfigured,
            waitingForTimer,
            waitingForEvent
        };

        priority_t mPriority = static_cast<priority_t>(-1);
        tick_t mScheduledDeadline = 0;

        // the member matching mState is the one last written
        union {
            tick_t mPeriod = 0;
            const TaskletEvent* mEvent;
        };
        eState mState = eState::unconfigured;
    };

}
