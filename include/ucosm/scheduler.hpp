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

namespace ucosm {

    /**
     * @brief Scheduler.
     *
     * @tparam task_rank_t Rank type of the tasks to schedule.
     * @tparam sched_rank_t Rank type if the scheduler.
     */
    template<typename task_rank_t, typename sched_rank_t>
    struct Scheduler : ITask<sched_rank_t> {

        using task_t = ITask<task_rank_t>;
        using get_rank_t = task_rank_t(*)(void);
        using idle_func_t = void(*)();

        /**
         * @brief Constructs a new scheduler object.
         *
         * @param inPolicy Function pointer of the policy to follow
         * for task execution.
         */
        Scheduler(get_rank_t inGetCurrentRank) :
            mGetCurrentRank(inGetCurrentRank) {
            mTasks.push_front(mCursorTask);
            mCursorTask.setRank(mGetCurrentRank());
        }

        /**
         * @brief Runs the scheduler.
         */
        void run() override;

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
         *            //task_t* next() { return this->next; }
         * @return true if the scheduler doesn't contain any task.
         * @return false otherwise.
         */
        bool empty() const;

        /**
         * @brief Removes every tasks from the scheduler.
         */
        void clear();

        /**
         * @brief Set the idle function object.
         *
         * @param inIdleUntil Function to call on idle.
         */
        void setIdleFunction(idle_func_t inIdleFunction);

        /**
         * @brief Pushes task names into a given stream.
         *
         * @tparam stream_t Stream type.
         * @param inStream Stream instance.
         * @param inSep Character used to separate task names.
         */
        template<typename stream_t>
        void list(stream_t&& inStream, std::string_view inSeparator = "\n");

    protected:

        ulink::List<task_t> mTasks;

        idle_func_t mIdleFunction = nullptr;

        task_t* mCurrentTask = nullptr;

        get_rank_t mGetCurrentRank;

        struct CursorTask : ITask<task_rank_t> {
            void run() final {}
            std::string_view name() const override { return ">"; }
        };

        CursorTask mCursorTask;

    };

    template<typename task_rank_t, typename sched_rank_t>
    void Scheduler<task_rank_t, sched_rank_t>::run() {

        using iterator_t = typename ulink::List<task_t>::iterator;

        iterator_t it(&mCursorTask);
        ++it;

        if (!mCursorTask.setRank(mGetCurrentRank())) {
            // no task to run
            if (mIdleFunction) {
                mIdleFunction();
            }
            return;
        }

        iterator_t it_rank(&mCursorTask);

        while (it != it_rank && it != mTasks.end()) {
            mCurrentTask = &(*it);
            ++it;
            mCurrentTask->run();
        }

        mCurrentTask = nullptr;

        if (it == it_rank) {
            // no more task to run
            return;
        }

        it = mTasks.begin();

        while (it != it_rank) {
            mCurrentTask = &(*it);
            ++it;
            mCurrentTask->run();
        }

        mCurrentTask = nullptr;

    }

    template<typename task_rank_t, typename sched_rank_t>
    bool Scheduler<task_rank_t, sched_rank_t>::addTask(task_t& inTask) {
        if (!inTask.init()) {
            return false;
        }
        mTasks.insert_after(&mCursorTask, inTask);
        inTask.setRank(mGetCurrentRank());
        return true;
    }

    template<typename task_rank_t, typename sched_rank_t>
    typename Scheduler<task_rank_t, sched_rank_t>::task_t*
        Scheduler<task_rank_t, sched_rank_t>::thisTask() {
        return mCurrentTask;
    }

    template<typename task_rank_t, typename sched_rank_t>
    bool Scheduler<task_rank_t, sched_rank_t>::empty() const {
        return (&mTasks.front() == &mTasks.back());
    }

    template<typename task_rank_t, typename sched_rank_t>
    void Scheduler<task_rank_t, sched_rank_t>::clear() {
        mTasks.clear();
        mTasks.push_front(mCursorTask);
    }

    template<typename task_rank_t, typename sched_rank_t>
    std::size_t Scheduler<task_rank_t, sched_rank_t>::size() const {
        return (mTasks.size() - 1); // remove rank task
    }

    template<typename task_rank_t, typename sched_rank_t>
    void Scheduler<task_rank_t, sched_rank_t>::setIdleFunction(idle_func_t inIdleFunction) {
        mIdleFunction = inIdleFunction;
    }

    template<typename task_rank_t, typename sched_rank_t>
    template<typename stream_t>
    void Scheduler<task_rank_t, sched_rank_t>::list(
        stream_t&& inStream,
        std::string_view inSeparator
    ) {
        for (const auto& t : mTasks) {
            inStream << t.name() << inSeparator;
        }
    }

}