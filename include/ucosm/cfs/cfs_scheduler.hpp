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

#include "core/ischeduler.hpp"
#include "icfs_task.hpp"

namespace ucosm {

    /**
     * @brief Completely fair scheduler.
     *
     * @tparam sched_rank_t Scheduler rank type.
     */
    template<typename sched_rank_t>
    struct CFSScheduler : IScheduler<ICFSTask, sched_rank_t> {

        using get_tick_t = ICFSTask::tick_t(*)();

        CFSScheduler(get_tick_t inGetTick) : mGetTick(inGetTick) {}

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
            if (this->mIdleFunction) {
                this->mIdleFunction();
            }
            return;
        }

        iterator_t it(&this->mCursorTask);
        ++it;

        if (it == this->mTasks.end()) {
            it = this->mTasks.begin();
        }

        this->mCurrentTask = static_cast<ICFSTask*>(&(*it));

        auto rank = mGetTick();

        this->mCurrentTask->run();

        rank = mGetTick() - rank;

        if (!this->mCurrentTask->isLinked()) {
            // task is not linked anymore : don't move the cursor task
            this->mCurrentTask = nullptr;
            return;
        }

        rank <<= this->mCurrentTask->getPriority();
        rank += this->mCurrentTask->getRank();

        if (!this->mCurrentTask->setRank(rank)) {
            // rank didn't change : don't move cursor task
            this->mCurrentTask = nullptr;
            return;
        }

        if (&this->mTasks.back() == &(*it)) {
            this->mTasks.push_front(this->mCursorTask);
        }
        else {
            this->mTasks.insert_after(it, this->mCursorTask);
        }

        this->mCurrentTask = nullptr;
    }

}