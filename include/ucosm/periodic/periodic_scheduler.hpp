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

#include "ucosm/core/deadline.hpp"
#include "ucosm/core/ischeduler.hpp"
#include "iperiodic_task.hpp"

namespace ucosm {

    using idle_task_t = void(*)();

    /**
     * @brief Periodic scheduler.
     *
     * @tparam sched_task_t Scheduler task type
     */
    template<typename sched_task_t = ITask<int8_t>>
    struct PeriodicScheduler : IScheduler<IPeriodicTask, sched_task_t> {

        using get_tick_t = IPeriodicTask::tick_t(*)();

        /**
         * @brief Construct a new periodic scheduler object.
         *
         * @param inGetTick Function returning the current tick.
         * @param inIdleTask Function to execute when there is no task to run.
         */
        PeriodicScheduler(get_tick_t inGetTick, idle_task_t inIdleTask = nullptr) :
            mIdleTask(inIdleTask),
            mGetTick(inGetTick) {}

        /**
         * @brief Adds a task to the scheduler, due right away.
         *
         * @param inTask Task instance.
         * @return true if the task was successfully added.
         * @return false otherwise.
         */
        virtual bool addTask(IPeriodicTask& inTask);

        /**
         * @brief Set the idle function.
         *
         * @param inIdleTask Function to call on idle.
         */
        void setIdleTask(idle_task_t inIdleTask);

        /**
         * @brief Sets the delay before the next execution of a scheduled task.
         * The period takes over afterwards.
         *
         * @param inTask Task instance.
         * @param inDelay Delay value.
         * @return true if the task was re-sorted.
         * @return false if the task isn't scheduled.
         */
        bool setDelay(IPeriodicTask& inTask, IPeriodicTask::tick_t inDelay);

        /**
         * @brief Runs the next ready tasks.
         */
        void run() override;

    protected:

        using base_t = IScheduler<IPeriodicTask, sched_task_t>;
        using typename base_t::itask_t;
        using typename base_t::task_rank_t;

        // Runs the task, then re-arms it one period after reference if still linked.
        void runAndRearm(IPeriodicTask& task, task_rank_t reference);

        idle_task_t mIdleTask;

        get_tick_t mGetTick;

    };

    template<typename sched_rank_t>
    bool PeriodicScheduler<sched_rank_t>::addTask(IPeriodicTask& inTask) {
        if (inTask.isLinked()) {
            return false;
        }

        const auto tick = mGetTick();

        if (this->empty()) {
            // bring an idle cursor to the current tick, keeping it wrap-safe
            this->mCursorTask.setRank(tick);
        }

        inTask.setRank(tick);
        this->insertByDeadline(inTask);
        return true;
    }

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::setIdleTask(idle_task_t inIdleTask) {
        mIdleTask = inIdleTask;
    }

    template<typename sched_rank_t>
    bool PeriodicScheduler<sched_rank_t>::setDelay(
        IPeriodicTask& inTask,
        IPeriodicTask::tick_t inDelay
    ) {
        if (!inTask.isLinked()) {
            return false;
        }

        inTask.setRank(makeDeadline(mGetTick(), inDelay));
        this->insertByDeadline(inTask);
        return true;
    }

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::run() {

        const auto tick = mGetTick();

        this->mCurrentTask = this->selectReadyTask(tick);

        if (!this->mCurrentTask) {
            if (mIdleTask) {
                mIdleTask();
            }
            return;
        }

        // catch-up semantics : re-armed from the current tick
        this->runAndRearm(*this->mCurrentTask, tick);

        this->mCurrentTask = nullptr;
    }

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::runAndRearm(IPeriodicTask& task, task_rank_t reference) {

        // earliest deadline in the list, so no task falls behind the cursor
        this->mCursorTask.setRank(task.getRank());

        task.run();

        if (task.isLinked()) {
            task.setRank(makeDeadline(reference, task.getPeriod()));

            this->insertByDeadline(task);
        }
    }

}