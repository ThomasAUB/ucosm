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
     * @tparam task_t Task type to schedule.
     * @tparam sched_task_t Scheduler task type
     */
    template<typename task_t, typename sched_task_t>
    struct IScheduler : sched_task_t {

        using idle_func_t = void(*)();

        /**
         * @brief Constructs a new scheduler object.
         *
         * @param inPolicy Function pointer of the policy to follow
         * for task execution.
         */
        IScheduler() {
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

        using task_rank_t = typename task_t::rank_t;
        using itask_t = ITask<task_rank_t>;

        ulink::List<itask_t> mTasks;

        idle_func_t mIdleFunction = nullptr;

        task_t* mCurrentTask = nullptr;

        struct CursorTask : itask_t {
            void run() final {}
            std::string_view name() override { return ">"; }
        };

        CursorTask mCursorTask;

    };

    template<typename task_t, typename sched_rank_t>
    bool IScheduler<task_t, sched_rank_t>::addTask(task_t& inTask) {
        if (!inTask.init()) {
            return false;
        }
        mTasks.insert_after(&mCursorTask, inTask);
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
    void IScheduler<task_t, sched_rank_t>::setIdleFunction(idle_func_t inIdleFunction) {
        mIdleFunction = inIdleFunction;
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

}