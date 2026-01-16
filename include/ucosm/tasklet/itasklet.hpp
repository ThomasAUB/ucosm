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

        void sleepFor(tick_t inSleepDuration) {
            mState = eState::sleeping;
            mSleepDuration = inSleepDuration;
            mInterruptID = invalid_interrupt_id;
        }

        void waitForInterrupt(interrupt_id_t inInterruptID) {
            mInterruptID = inInterruptID;
            mState = eState::waitingForInterrupt;
            mSleepDuration = 0;
        }

        void dispose() {
            mState = eState::waitingForInterrupt;
            mInterruptID = invalid_interrupt_id;
            mSleepDuration = 0;
        }

        tick_t getSleepDuration() const {
            return mSleepDuration;
        }

        bool isSleeping() const {
            return mState == eState::sleeping;
        }

        bool isWaitingForInterrupt() const {
            return mState == eState::waitingForInterrupt;
        }

        interrupt_id_t getInterruptID() const {
            return mInterruptID;
        }

        ~ITasklet() = default;

    private:

        enum class eState : uint8_t {
            sleeping,
            waitingForInterrupt
        };

        static constexpr interrupt_id_t invalid_interrupt_id = static_cast<interrupt_id_t>(-1);

        priority_t mPriority = static_cast<priority_t>(-1);
        interrupt_id_t mInterruptID = invalid_interrupt_id;
        tick_t mSleepDuration = 0;
        eState mState = eState::sleeping;
    };

}