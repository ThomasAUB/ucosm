#pragma once

#include "core/ischeduler.hpp"
#include "iperiodic_task.hpp"

namespace ucosm {

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

        void run() override;

    private:

        void postRun(IPeriodicTask::tick_t inCurrentTick) override;

        get_tick_t mGetTick;

    };

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::run() {
        this->runUntilCursor(mGetTick());
    }

    template<typename sched_rank_t>
    void PeriodicScheduler<sched_rank_t>::postRun(IPeriodicTask::tick_t inCurrentTick) {
        //this->mCurrentTask->setRank(inCurrentTick + this->mCurrentTask->getPeriod());
    }

}