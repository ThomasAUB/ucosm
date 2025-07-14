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

#include "ucosm/core/ischeduler.hpp"
#include "iperiodic_task.hpp"

namespace ucosm {

    /**
     * @brief Periodic scheduler.
     *
     * @tparam sched_task_t Scheduler task type
     */
    template<typename sched_task_t = ITask<int8_t>>
    struct PeriodicScheduler : IScheduler<IPeriodicTask, sched_task_t> {

        using get_tick_t = IPeriodicTask::tick_t(*)();

        PeriodicScheduler(get_tick_t inGetTick, idle_task_t inIdleTask = nullptr) :
            IScheduler<IPeriodicTask, sched_task_t>(inIdleTask),
            mGetTick(inGetTick) {}

        /**
         * @brief Delay the task.
         *
         * @param inDelay Delay value.
         */
        void setDelay(IPeriodicTask& inTask, IPeriodicTask::tick_t inDelay);

        /**
         * @brief Runs the next ready tasks.
         */
        void run() override;

    protected:

        get_tick_t mGetTick;

    };

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::setDelay(
        IPeriodicTask& inTask,
        IPeriodicTask::tick_t inDelay
    ) {
        inTask.setRank(mGetTick() + inDelay);
        this->sortTask(inTask);
    }

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::run() {

        const auto tick = mGetTick();

        this->mCurrentTask = this->getNextTask();

        if (this->mCurrentTask) {

            const auto cursorRank = this->mCursorTask.getRank();
            const auto deltaTask = this->mCurrentTask->getRank() - cursorRank;
            const auto deltaTick = tick - cursorRank;

            if (deltaTick < deltaTask) {
                // task is not ready
                this->mCurrentTask = nullptr;
            }

        }

        if (!this->mCurrentTask) {
            // no task to run
            if (this->mIdleTask) {
                this->mIdleTask();
            }
            return;
        }

        this->mCursorTask.setRank(this->mCurrentTask->getRank());
        this->mCurrentTask->run();

        // Check if task is still linked after execution
        if (this->mCurrentTask->isLinked()) {

            // the task is still in the list
            // update the task rank
            this->mCurrentTask->setRank(
                tick +
                this->mCurrentTask->getPeriod()
            );

            this->sortTask(*this->mCurrentTask);
        }

        this->mCurrentTask = nullptr;
    }

}