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

#include "ucosm/periodic/iperiodic_task.hpp"
#include "ucosm/tasklet/itasklet.hpp"

#define UCOSM_START                                         \
    do {                                                    \
        switch(this->mLine) {                               \
        case ucosm::detail::resumable_init_state: {



#define UCOSM_SLEEP_FOR(tick)                               \
            this->setPeriod(static_cast<ucosm::tick_t>(tick)); \
            this->mLine = __LINE__;                         \
            return;                                         \
        }                                                   \
        case __LINE__: {                                    \
            this->setPeriod(0);



#define UCOSM_YIELD UCOSM_SLEEP_FOR(0)



#define UCOSM_SLEEP_UNTIL(condition, check_period)          \
        if(!(condition)) {                                  \
            UCOSM_SLEEP_FOR(check_period);                  \
            if (!(condition)) {                             \
                this->setPeriod(check_period);              \
                return;                                     \
            }                                               \
        }



// Tasklet only : suspends until the event is signaled.
#define UCOSM_WAIT_EVENT(event_id)                          \
            this->waitForEvent(event_id);                   \
            this->mLine = __LINE__;                         \
            return;                                         \
        }                                                   \
        case __LINE__: {



#define UCOSM_RESTART                                       \
            this->mLine = ucosm::detail::resumable_init_state; \
            return;               



#define UCOSM_END                                           \
            this->mLine = ucosm::detail::resumable_init_state; \
            this->removeTask();                             \
            return;                                         \
        }                                                   \
        default:                                            \
            /* Invalid state - task corrupted */            \
            this->removeTask();                             \
            return;                                         \
        }                                                   \
    } while(0);



namespace ucosm {

    namespace detail {
        // Not a member, so that UCOSM_START also finds it from a templated task.
        static constexpr int resumable_init_state = -1;
    }

    /**
     * @brief Coroutine-like task, written with the UCOSM_* macros.
     * On a tasklet, UCOSM_YIELD sleeps for one tick.
     *
     * @tparam base_t IPeriodicTask or ITasklet.
     */
    template<typename base_t>
    struct ResumableTask : base_t {

        using base_t::base_t;

        /**
         * @brief Reset task to initial state, e.g. to restart a completed task.
         */
        void reset() {
            mLine = detail::resumable_init_state;
            this->setPeriod(0);
        }

    protected:
        int mLine = detail::resumable_init_state;
    };

    using IResumableTask = ResumableTask<IPeriodicTask>;
    using IResumableTasklet = ResumableTask<ITasklet>;

}