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
#include "icfs_task.hpp"

namespace ucosm {

    /**
     * @brief Completely fair scheduler.
     *
     * @tparam sched_task_t Scheduler task type
     */
    template<typename sched_task_t = ITask<int8_t>>
    struct CFSScheduler : IScheduler<ICFSTask, sched_task_t> {

        using get_tick_t = ICFSTask::tick_t(*)();

        CFSScheduler(get_tick_t inGetTick, idle_task_t inIdleTask = nullptr) :
            IScheduler<ICFSTask, sched_task_t>(inIdleTask),
            mGetTick(inGetTick) {}

        /**
         * @brief Runs the task that has the lower execution time.
         */
        void run() override;

    private:

        get_tick_t mGetTick;

    };

    template<typename sched_rank_t>
    void CFSScheduler<sched_rank_t>::run() {

        using iterator_t = typename decltype(this->mTasks)::iterator;

        if (this->empty()) {
            // no task to run
            if (this->mIdleTask) {
                this->mIdleTask();
            }
            return;
        }

        {
            if (&this->mCursorTask == &this->mTasks.back()) {
                this->mCurrentTask = static_cast<ICFSTask*>(&this->mTasks.front());
            }
            else {
                this->mCurrentTask = this->mCursorTask.next();
            }

            auto rank = mGetTick();

            this->mCurrentTask->run();

            rank = mGetTick() - rank;

            rank <<= this->mCurrentTask->getPriority();
            rank += this->mCurrentTask->getRank();
            this->mCurrentTask->setRank(rank);

            if (!this->sortTask(*this->mCurrentTask)) {
                // task is not linked anymore or
                // rank didn't change
                // -> don't move cursor task
                this->mCurrentTask = nullptr;
                return;
            }

        }

        // move cursor task right before the next one
        ICFSTask::rank_t newRank;

        if (&this->mCursorTask == &this->mTasks.back()) {
            newRank = this->mTasks.front().getRank();
        }
        else {
            newRank = this->mCursorTask.next()->getRank();
        }

        this->mCursorTask.setRank(newRank);
        this->sortTask(this->mCursorTask);
        this->mCurrentTask = nullptr;
    }

}