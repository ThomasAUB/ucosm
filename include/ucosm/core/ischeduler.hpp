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

    /**
     * @brief Base scheduler : a deadline-sorted task list, ordered from a cursor.
     * The cursor must never move past a deadline still in the list.
     *
     * @tparam task_t Task type to schedule.
     * @tparam sched_task_t Scheduler task type
     */
    template<typename task_t, typename sched_task_t>
    struct IScheduler : sched_task_t {

        /**
         * @brief Returns the currently executed task.
         *
         * @return task_t* Pointer to the task, nullptr outside of a task
         * execution.
         */
        task_t* thisTask();

        /**
         * @brief Returns the number of task in the scheduler.
         * Walks the whole task list.
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
         * @brief Pushes task names into a given stream.
         *
         * @tparam stream_t Stream type.
         * @param inStream Stream instance.
         * @param inSeparator String used to separate task names.
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

        // Returns the task with the earliest deadline, or nullptr if none.
        task_t* getNextTask();

        // Sorts a task into the list, wherever it was before. Ordered by delay
        // from the cursor rather than raw rank, so that wrapped deadlines sort right.
        void insertByDeadline(itask_t& inTask);

        // Tells if a deadline is due at inNow, wrap-safe thanks to the cursor.
        bool isDue(task_rank_t inDeadline, task_rank_t inNow) const;

        // Returns the next task if it is due at inNow, nullptr otherwise.
        task_t* selectReadyTask(task_rank_t inNow);

        ulink::List<itask_t> mTasks;

        task_t* mCurrentTask = nullptr;

        // reference tick of the wrap-safe deadline comparisons
        task_rank_t mCursor = task_rank_t();

    };

    template<typename task_t, typename sched_rank_t>
    task_t* IScheduler<task_t, sched_rank_t>::thisTask() {
        return mCurrentTask;
    }

    template<typename task_t, typename sched_rank_t>
    bool IScheduler<task_t, sched_rank_t>::empty() const {
        return mTasks.empty();
    }

    template<typename task_t, typename sched_rank_t>
    void IScheduler<task_t, sched_rank_t>::clear() {
        mTasks.clear();
    }

    template<typename task_t, typename sched_rank_t>
    std::size_t IScheduler<task_t, sched_rank_t>::size() const {
        return mTasks.size();
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

        if (empty()) {
            return 0;
        }

        return mTasks.front().getRank();
    }

    template<typename task_t, typename sched_rank_t>
    task_t* IScheduler<task_t, sched_rank_t>::getNextTask() {

        if (empty()) {
            return nullptr;
        }

        return &static_cast<task_t&>(mTasks.front());
    }

    template<typename task_t, typename sched_rank_t>
    void IScheduler<task_t, sched_rank_t>::insertByDeadline(itask_t& inTask) {

        using task_list_t = ulink::List<itask_t>;

        if (mTasks.empty()) {
            mTasks.push_back(inTask);
            return;
        }

        const auto cursor = mCursor;
        const auto delay = getDeadlineDelay(cursor, inTask.getRank());

        // Walked from the closest end, keeping FIFO order for equal deadlines.
        // The task itself is skipped : it may still be linked here.
        auto& back = *mTasks.rbegin();
        const auto backDelay = getDeadlineDelay(cursor, back.getRank());

        if (&back != &inTask && delay >= backDelay) {
            mTasks.push_back(inTask);
            return;
        }

        if (delay <= backDelay / 2) {
            auto it = mTasks.begin();
            const auto endIt = mTasks.end();

            // the task itself compares equal, so it is walked past
            while (it != endIt && getDeadlineDelay(cursor, it->getRank()) <= delay) {
                ++it;
            }

            mTasks.insert_before(it, inTask);
        }
        else {
            typename task_list_t::reverse_iterator it = mTasks.rbegin();
            const auto rendIt = mTasks.rend();

            while (it != rendIt &&
                (&*it == &inTask || getDeadlineDelay(cursor, it->getRank()) > delay)) {
                ++it;
            }

            if (it == rendIt) {
                mTasks.push_front(inTask);
            }
            else {
                mTasks.insert_after(typename task_list_t::iterator(&*it), inTask);
            }
        }
    }

    template<typename task_t, typename sched_rank_t>
    bool IScheduler<task_t, sched_rank_t>::isDue(
        task_rank_t inDeadline,
        task_rank_t inNow
    ) const {
        return isDeadlineDue(mCursor, inDeadline, inNow);
    }

    template<typename task_t, typename sched_rank_t>
    task_t* IScheduler<task_t, sched_rank_t>::selectReadyTask(task_rank_t inNow) {

        auto* candidate = getNextTask();

        if (candidate && !isDue(candidate->getRank(), inNow)) {
            return nullptr;
        }

        return candidate;
    }

}