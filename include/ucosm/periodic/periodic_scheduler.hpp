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
#include "iperiodic_task.hpp"

namespace ucosm {

    /**
     * @brief Periodic scheduler.
     *
     * @tparam sched_rank_t Scheduler rank type
     */
    template<typename sched_rank_t>
    struct PeriodicScheduler : IScheduler<IPeriodicTask, sched_rank_t> {

        using get_tick_t = IPeriodicTask::tick_t(*)();

        PeriodicScheduler(get_tick_t inGetTick) : mGetTick(inGetTick) {}

        /**
         * @brief Delay the task.
         *
         * @param inDelay Delay value.
         */
        void setDelay(IPeriodicTask& inTask, IPeriodicTask::tick_t inDelay) { inTask.setRank(mGetTick() + inDelay); }

        /**
         * @brief Runs every ready tasks.
         */
        void run() override;

    private:

        get_tick_t mGetTick;

    };

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::run() {

        using iterator_t =
            typename IScheduler<IPeriodicTask, sched_rank_t>::iterator_t;


        iterator_t it(&this->mCursorTask);

        ++it;

        const auto tick = mGetTick();

        if (!this->mCursorTask.setRank(tick)) {
            // no task to run
            if (this->mIdleFunction) {
                this->mIdleFunction();
            }
            return;
        }

        iterator_t it_rank(&this->mCursorTask);

        while (it != it_rank && it != this->mTasks.end()) {
            this->mCurrentTask = static_cast<IPeriodicTask*>(&(*it));
            ++it;
            this->mCurrentTask->run();
            this->mCurrentTask->setRank(tick + this->mCurrentTask->getPeriod());
        }

        this->mCurrentTask = nullptr;

        if (it == it_rank) {
            // no more task to run
            return;
        }

        it = this->mTasks.begin();

        while (it != it_rank) {
            this->mCurrentTask = static_cast<IPeriodicTask*>(&(*it));
            ++it;
            this->mCurrentTask->run();
            this->mCurrentTask->setRank(tick + this->mCurrentTask->getPeriod());
        }

        this->mCurrentTask = nullptr;

    }

}