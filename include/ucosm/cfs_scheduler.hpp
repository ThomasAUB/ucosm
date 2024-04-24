#pragma once

#include "scheduler.hpp"
#include "icfs_task.hpp"

namespace ucosm {

    template<typename sched_rank_t>
    struct CFSScheduler : Scheduler<ICFSTask::tick_t, sched_rank_t> {

        void run() override;

    };

    template<typename sched_rank_t>
    void CFSScheduler<sched_rank_t>::run() {

        if (this->empty()) {
            // no task to run
            if (this->mIdleFunction) {
                this->mIdleFunction();
            }
            return;
        }

        iterator_t it(&this->mCursorTask);
        ++it;

        if (it == mTasks.end()) {
            it = mTasks.begin();
        }

        const auto rank = (*it).getRank() + 1;

        if (this->mCurrentTask.setRank(rank)) {
            // no task to run
            if (mIdleFunction) {
                mIdleFunction();
            }
            return;
        }

        this->runUntilCursor(it);
    }

}