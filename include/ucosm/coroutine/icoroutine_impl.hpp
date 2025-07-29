/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2024 Thomas AUBERT                                                *
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

#include "icoroutine.hpp"
#include "coroutine_scheduler.hpp"

namespace ucosm {

    template<std::size_t StackSize>
    void ICoroutine<StackSize>::waitFor(tick_t ticks) {
        if (mCurrentScheduler && ticks > 0) {
            mWaitUntil = mCurrentScheduler->getCurrentTick() + ticks;
            mState = CoroutineState::Waiting;
            contextSwitch(&mCoroutineContext, &mSchedulerContext);
        }
    }

    template<std::size_t StackSize>
    void ICoroutine<StackSize>::waitUntil(tick_t target_tick) {
        if (mCurrentScheduler && target_tick > mCurrentScheduler->getCurrentTick()) {
            mWaitUntil = target_tick;
            mState = CoroutineState::Waiting;
            contextSwitch(&mCoroutineContext, &mSchedulerContext);
        }
    }

    template<std::size_t StackSize>
    void ICoroutine<StackSize>::run() {
        if (mState == CoroutineState::Finished) {
            this->removeTask();
            return;
        }

        // Check if we're still waiting
        if (mState == CoroutineState::Waiting) {
            if (mCurrentScheduler && 
                mCurrentScheduler->getCurrentTick() < mWaitUntil) {
                return; // Still waiting
            }
            mState = CoroutineState::Suspended; // Ready to resume
        }

        // Switch to coroutine context
        mState = CoroutineState::Running;
        contextSwitch(&mSchedulerContext, &mCoroutineContext);
        
        // We return here when the coroutine yields or finishes
        if (mState == CoroutineState::Running) {
            mState = CoroutineState::Suspended;
        }
    }

} // namespace ucosm