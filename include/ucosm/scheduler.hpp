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

    template<typename task_rank_t, typename sched_rank_t>
    struct Scheduler : ITask<sched_rank_t> {

        using task_t = ITask<task_rank_t>;
        using policy_t = bool(*)(task_rank_t);

        Scheduler(policy_t inPolicy) :
            mPolicy(inPolicy) {}

        void run() override;

        virtual bool addTask(task_t& inTask);

        std::size_t size() const;

        bool empty() const;

        void clear();

        template<typename stream_t>
        void list(stream_t&& inStream, const char inSep = '\n');

    protected:
        policy_t mPolicy;
        ulink::List<task_t> mTasks;
    };

    template<typename task_rank_t, typename sched_rank_t>
    void Scheduler<task_rank_t, sched_rank_t>::run() {
        if (!mTasks.empty() && mPolicy(mTasks.front().getRank())) {
            mTasks.front().run();
        }
    }

    template<typename task_rank_t, typename sched_rank_t>
    bool Scheduler<task_rank_t, sched_rank_t>::addTask(task_t& inTask) {
        if (!inTask.init()) {
            return false;
        }
        mTasks.push_front(inTask);
        inTask.updateRank();
        return true;
    }

    template<typename task_rank_t, typename sched_rank_t>
    bool Scheduler<task_rank_t, sched_rank_t>::empty() const {
        return mTasks.empty();
    }

    template<typename task_rank_t, typename sched_rank_t>
    void Scheduler<task_rank_t, sched_rank_t>::clear() {
        return mTasks.clear();
    }

    template<typename task_rank_t, typename sched_rank_t>
    std::size_t Scheduler<task_rank_t, sched_rank_t>::size() const {
        return mTasks.size();
    }

    template<typename task_rank_t, typename sched_rank_t>
    template<typename stream_t>
    void Scheduler<task_rank_t, sched_rank_t>::list(
        stream_t&& inStream,
        const char inSep
    ) {
        for (const auto& t : mTasks) {
            inStream << t.name() << ":" << t.getRank() << inSep;
        }
    }

}