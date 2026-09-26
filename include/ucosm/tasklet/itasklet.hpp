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
    using event_id_t = uint8_t;

    template<event_id_t event_count>
    struct TaskletScheduler;

    struct ITasklet : ITask<priority_t> {

        void setPriority(priority_t inPriority) {
            mPriority = inPriority;
        }

        priority_t getPriority() const {
            return mPriority;
        }

        void setPeriod(tick_t inPeriod) {
            mState = eState::waitingForTimer;
            mWakeupSource = inPeriod > 0 ? inPeriod : 1;
        }

        // 0 when the task is not waiting for the timer.
        tick_t getPeriod() const {
            return isWaitingForTimer() ? mWakeupSource : 0;
        }

        void waitForEvent(event_id_t inEventID) {
            mState = eState::waitingForEvent;
            mWakeupSource = inEventID;
        }

        void dispose() {
            mState = eState::unconfigured;
            mWakeupSource = 0;
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

        // invalid_event_id when the task is not waiting for an event.
        event_id_t getEventID() const {
            return isWaitingForEvent() ?
                static_cast<event_id_t>(mWakeupSource) :
                invalid_event_id;
        }

        ~ITasklet() = default;

    protected:

        // Demoted from ITask's public removeTask() : use
        // TaskletScheduler::removeTask() from outside, or this->removeTask()
        // from within run().
        using ITask<priority_t>::removeTask;

    private:

        template<event_id_t>
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

        static constexpr event_id_t invalid_event_id = static_cast<event_id_t>(-1);

        priority_t mPriority = static_cast<priority_t>(-1);
        tick_t mScheduledDeadline = 0;

        // A task waits either for the timer or for an event, never both :
        // holds the period or the event id, as mState tells. Read it
        // through getPeriod() / getEventID(), which check mState.
        tick_t mWakeupSource = 0;
        eState mState = eState::unconfigured;
    };

}