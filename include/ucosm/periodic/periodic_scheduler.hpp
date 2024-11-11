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
#include <limits>

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
        void setDelay(IPeriodicTask& inTask, IPeriodicTask::tick_t inDelay) {
            inTask.setRank(mGetTick() + inDelay);
        }

        /**
         * @brief Runs the next ready tasks.
         */
        void run() override;

    protected:

        IPeriodicTask* getNextReadyTask();

        get_tick_t mGetTick;

    };

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::run() {

        this->mCurrentTask = getNextReadyTask();

        if (!this->mCurrentTask) {
            // no task to run
            if (this->mIdleTask) {
                this->mIdleTask();
            }
            return;
        }

        // store the rank in case the task edits it during execution
        const auto taskRank = this->mCurrentTask->getRank();

        this->mCurrentTask->run();

        // set the cursor task's rank to the current one
        this->mCursorTask.setRank(taskRank);

        if (this->mCurrentTask->isLinked()) {

            // the task is still in the list
            // update the task rank

            const auto tick = mGetTick();

            const auto taskPeriod = this->mCurrentTask->getPeriod();

            // insert task where it should have the least work to reorder
            if (std::numeric_limits<IPeriodicTask::rank_t>::max() - tick < taskPeriod) {
                // rank overflow : place task to front
                this->mTasks.push_front(*this->mCurrentTask);
            }

            this->mCurrentTask->setRank(tick + taskPeriod);
        }

        this->mCurrentTask = nullptr;
    }

    template<typename sched_rank_t>
    IPeriodicTask* PeriodicScheduler<sched_rank_t>::getNextReadyTask() {

        if (this->empty()) {
            return nullptr;
        }

        using iterator_t = typename decltype(this->mTasks)::iterator;

        iterator_t it(&this->mCursorTask);

        ++it;

        const auto currentTick = mGetTick();

        if (it == this->mTasks.end()) {

            // check for tick overflow

            if (currentTick >= this->mCursorTask.getRank()) {
                // tick didn't overflow
                return nullptr;
            }

            it = this->mTasks.begin();
        }

        if (currentTick < (*it).getRank()) {
            return nullptr;
        }

        return static_cast<IPeriodicTask*>(&(*it));
    }

}