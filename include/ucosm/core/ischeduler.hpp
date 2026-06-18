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

#include "ulink.hpp"
#include "itask.hpp"
#include "deadline.hpp"

namespace ucosm {

    using idle_task_t = void(*)();

    /**
     * @brief Scheduler.
     *
     * @tparam task_t Task type to schedule.
     * @tparam sched_task_t Scheduler task type
     */
    template<typename task_t, typename sched_task_t>
    struct IScheduler : sched_task_t {

        /**
         * @brief Construct a new scheduler object.
         *
         * @param inIdleTask Function to execute when there is no task to run.
         */
        IScheduler(idle_task_t inIdleTask = nullptr) :
            mIdleTask(inIdleTask) {
            mTasks.push_front(mCursorTask);
        }

        /**
         * @brief Adds a task to the scheduler.
         *
         * @param inTask Task instance.
         * @return true if the task was successfully added.
         * @return false otherwise.
         */
        virtual bool addTask(task_t& inTask);

        /**
         * @brief Returns the currently executed task.
         *
         * @return task_t* Pointer to the task.
         */
        task_t* thisTask();

        /**
         * @brief Returns the number of task in the scheduler.
         * This function will traverse the task list in order to count them.
         * This function might perform poorly if the scheduler contains a lot of tasks.
         *
         * @return std::size_t Number of task.
         */
        std::size_t size() const;

        /**
         * @brief Tells if the scheduler contains any task.
         *
         * @return true if the scheduler doesn't contain any task.
         * @return false otherwise.
         */
        bool empty() const;

        /**
         * @brief Removes every tasks from the scheduler.
         */
        void clear();

        /**
         * @brief Set the idle function.
         *
         * @param inIdleTask Function to call on idle.
         */
        void setIdleTask(idle_task_t inIdleTask);

        /**
         * @brief Pushes task names into a given stream.
         *
         * @tparam stream_t Stream type.
         * @param inStream Stream instance.
         * @param inSep Character used to separate task names.
         */
        template<typename stream_t>
        void list(stream_t&& inStream, std::string_view inSeparator = "\n");

        /**
         * @brief Get the rank of the next task to be run.
         *
         * @return task_t::rank_t Rank value.
         */
        typename task_t::rank_t getNextRank() const;

    protected:

        using task_rank_t = typename task_t::rank_t;
        using itask_t = ITask<task_rank_t>;
        using const_task_iterator = typename ulink::List<itask_t>::const_iterator;

        bool sortTask(itask_t& inTask);

        task_t* getNextTask();

        // Selects the next task from the list and verifies its deadline is
        // due with respect to `now`. Returns the ready task, or nullptr if
        // no task is ready. On a not-ready candidate, the cursor is left in
        // place so getNextRank() reports the earliest pending deadline.
        task_t* selectReadyTask(task_rank_t now);

        // Runs the given task, then if it is still linked re-arms its rank
        // to makeDeadline(reference, task.getPeriod()) and re-sorts it.
        // `reference` is the time base for the new deadline: callers pass
        // the current tick for "catch-up" semantics (Periodic) or the
        // executed task's previous rank for "drift-free" semantics (RT).
        void runAndRearm(task_t& task, task_rank_t reference);

        ulink::List<itask_t> mTasks;

        idle_task_t mIdleTask;

        task_t* mCurrentTask = nullptr;

        struct CursorTask final : itask_t {
            void run() override {}
            std::string_view name() override { return ">"; }
            auto* next() { return static_cast<task_t*>(this->itask_t::next); }
            const auto* next() const { return static_cast<const task_t*>(this->itask_t::next); }
        };

        CursorTask mCursorTask;

    };

    template<typename task_t, typename sched_rank_t>
    bool IScheduler<task_t, sched_rank_t>::addTask(task_t& inTask) {
        // Check if task is already linked to prevent double-adding
        if (inTask.isLinked()) {
            return false;
        }

        if (!inTask.init()) {
            return false;
        }

        mTasks.insert_after(&mCursorTask, inTask);
        inTask.setRank(mCursorTask.getRank());
        this->sortTask(inTask);
        return true;
    }

    template<typename task_t, typename sched_rank_t>
    task_t* IScheduler<task_t, sched_rank_t>::thisTask() {
        return static_cast<task_t*>(mCurrentTask);
    }

    template<typename task_t, typename sched_rank_t>
    bool IScheduler<task_t, sched_rank_t>::empty() const {
        return (&mTasks.front() == &mTasks.back());
    }

    template<typename task_t, typename sched_rank_t>
    void IScheduler<task_t, sched_rank_t>::clear() {
        mTasks.clear();
        mTasks.push_front(mCursorTask);
    }

    template<typename task_t, typename sched_rank_t>
    std::size_t IScheduler<task_t, sched_rank_t>::size() const {
        return (mTasks.size() - 1);
    }

    template<typename task_t, typename sched_rank_t>
    void IScheduler<task_t, sched_rank_t>::setIdleTask(idle_task_t inIdleTask) {
        mIdleTask = inIdleTask;
    }

    template<typename task_t, typename sched_rank_t>
    template<typename stream_t>
    void IScheduler<task_t, sched_rank_t>::list(
        stream_t&& inStream,
        std::string_view inSeparator
    ) {
        for (auto& t : mTasks) {
            inStream << t.name() << inSeparator;
        }
    }

    template<typename task_t, typename sched_rank_t>
    typename task_t::rank_t IScheduler<task_t, sched_rank_t>::getNextRank() const {

        auto& front = mTasks.front();
        auto& back = mTasks.back();

        if (&front == &back) {
            // only the cursor task is in the list
            return 0;
        }

        if (&this->mCursorTask == &back) {
            return this->mTasks.front().getRank();
        }
        else {
            return this->mCursorTask.next()->getRank();
        }

    }

    template<typename task_t, typename sched_rank_t>
    task_t* IScheduler<task_t, sched_rank_t>::getNextTask() {

        auto& front = mTasks.front();
        auto& back = mTasks.back();

        if (&front == &back) {
            // only the cursor task is in the list
            return nullptr;
        }

        if (&mCursorTask == &back) {
            // cursor is last
            // update cursor
            mTasks.push_front(mCursorTask);
            return static_cast<task_t*>(&front);
        }
        else {
            return mCursorTask.next();
        }

    }

    template<typename task_t, typename sched_rank_t>
    bool IScheduler<task_t, sched_rank_t>::sortTask(itask_t& inTask) {

        const auto rank = inTask.getRank();

        if (rank < mTasks.front().getRank()) {

            mTasks.push_front(inTask);

        }
        else if (rank > mTasks.back().getRank()) {

            mTasks.push_back(inTask);

        }
        else {
            return inTask.updateRank();
        }

        return true;

    }

    template<typename task_t, typename sched_rank_t>
    task_t* IScheduler<task_t, sched_rank_t>::selectReadyTask(task_rank_t now) {

        auto* candidate = getNextTask();

        if (candidate) {
            const auto cursorRank = mCursorTask.getRank();
            if (!isDeadlineDue(cursorRank, candidate->getRank(), now)) {
                // task is not ready yet; leave the cursor in place so
                // getNextRank() keeps reporting the earliest pending deadline.
                return nullptr;
            }
        }

        return candidate;
    }

    template<typename task_t, typename sched_rank_t>
    void IScheduler<task_t, sched_rank_t>::runAndRearm(task_t& task, task_rank_t reference) {

        // Advance the cursor to the executed task's rank so subsequent
        // deadline comparisons stay wrap-safe.
        mCursorTask.setRank(task.getRank());

        task.run();

        // Check if task is still linked after execution
        if (task.isLinked()) {

            // the task is still in the list
            // update the task rank
            task.setRank(makeDeadline(reference, task.getPeriod()));

            sortTask(task);
        }
    }

}