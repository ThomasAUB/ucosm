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
         * @brief Tells if other tasks are ready to be executed
         *
         * @return true if other tasks are ready.
         * @return false otherwise.
         */
        bool hasWork() const;

        /**
         * @brief Runs every ready tasks.
         */
        void run() override;

    private:

        get_tick_t mGetTick;

    };

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::run() {

        using iterator_t = typename decltype(this->mTasks)::iterator;

        iterator_t it(&this->mCursorTask);

        ++it;

        const auto tick = mGetTick();

        if (!this->mCursorTask.setRank(tick)) {
            // no task to run
            if (this->mIdleTask) {
                this->mIdleTask();
            }
            return;
        }

        iterator_t cursor(&this->mCursorTask);

        auto runAll = [&] () {

            auto end = this->mTasks.end();

            while (it != cursor && it != end) {

                this->mCurrentTask = static_cast<IPeriodicTask*>(&(*it));

                ++it;

                this->mCurrentTask->run();

                if (this->mCurrentTask->isLinked()) {

                    // reinsertion optim
                    // insert task where it should have the least work to reorder

                    const auto taskPeriod = this->mCurrentTask->getPeriod();

                    if (std::numeric_limits<rank_t>::max() - tick < taskPeriod) {
                        // rank overflow : place task to front
                        this->mTasks.push_front(*this->mCurrentTask);
                    }
                    else {
                        // place task after the cursor
                        this->mTasks.insert_after(&this->mCursorTask, *this->mCurrentTask);
                    }

                    this->mCurrentTask->setRank(tick + taskPeriod);
                }
            }

            };

        runAll();

        this->mCurrentTask = nullptr;

        if (it == cursor) {
            // no more task to run
            return;
        }

        it = this->mTasks.begin();

        runAll();

        this->mCurrentTask = nullptr;
    }

    template<typename sched_rank_t>
    bool PeriodicScheduler<sched_rank_t>::hasWork() const {

        using iterator_t = typename decltype(this->mTasks)::const_iterator;

        if (this->empty()) {
            return false;
        }

        if (this->mCurrentTask) {

            // a task is currently running
            // check if next task must be run

            if (this->mTasks.size() == 1) {
                // current task is the only one and is being executed
                return false;
            }

            iterator_t it(this->mCurrentTask);

            ++it;

            if (it == this->mTasks.end()) {
                it = this->mTasks.begin();
            }

            if (it != &this->mCursorTask) {
                // at least one other task is waiting
                return true;
            }

        }

        // check from cursor task

        iterator_t it(&this->mCursorTask);
        ++it;

        const auto currentTick = mGetTick();

        if (it == this->mTasks.end()) {
            // check for tick overflow
            return
                (currentTick < this->mCursorTask.getRank()) &&      // tick overflow
                (currentTick >= this->mTasks.front().getRank());    // next task is ready
        }

        return (currentTick >= (*it).getRank());
    }

}