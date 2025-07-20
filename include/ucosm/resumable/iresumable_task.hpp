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

#define UCOSM_START                                         \
    do {                                                    \
        switch(this->mLine) {                               \
        case init_task_state: {



#define UCOSM_WAIT(tick)                                    \
            this->setPeriod(static_cast<tick_t>(tick));     \
            this->mLine = __LINE__;                         \
            return;                                         \
        }                                                   \
        case __LINE__: {                                    \
            this->setPeriod(0);



#define UCOSM_YIELD UCOSM_WAIT(0)



#define UCOSM_WAIT_UNTIL(condition, check_period)           \
        if(!(condition)) {                                  \
            UCOSM_WAIT(check_period);                       \
            if (!(condition)) {                             \
                return;                                     \
            }                                               \
        }



#define UCOSM_RESTART                                       \
            this->mLine = init_task_state;                  \
            return;               



#define UCOSM_END                                           \
            this->mLine = init_task_state;                  \
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

    /**
     * @brief Resumable task with coroutine-like behavior.
     *
     * Allows writing tasks that can yield execution and resume later,
     * similar to cooperative multitasking or coroutines.
     *
     * Usage:
     * @code
     * struct MyTask : IResumableTask {
     *     void run() override {
     *         UCOSM_START
     *             // Initialization code
     *             UCOSM_WAIT(1000)  // Wait 1 second
     *             // More code
     *             UCOSM_YIELD       // Yield to other tasks
     *             // Final code
     *         UCOSM_END
     *     }
     * };
     * @endcode
     */
    struct IResumableTask : IPeriodicTask {

        /**
         * @brief Reset task to initial state.
         * Useful for restarting completed tasks.
         */
        void reset() {
            mLine = init_task_state;
            this->setPeriod(0);
        }

    protected:
        static constexpr int init_task_state = -1;
        int mLine = init_task_state;
    };

}