#include "rt_scheduler.hpp"

namespace ucosm {

    RTScheduler::RTScheduler(ITimer& inTimer) :
        mTimer(inTimer) {
        mTimer.mScheduler = this;
    }

    bool RTScheduler::addTask(IPeriodicTask& inTask) {

        if (inTask.getPeriod() == 0) {
            // Invalid period
            return false;
        }

        mTimer.disableInterruption();

        const bool isTaskAdded = base_t::addTask(inTask);

        if (isTaskAdded && !mTimer.isRunning()) {
            mTimer.setDuration(0);
            mTimer.start();
        }

        mTimer.enableInterruption();

        return isTaskAdded;
    }

    void RTScheduler::run() {

        this->mCurrentTask = this->getNextTask();

        if (!this->mCurrentTask) {
            mTimer.stop();
            return;
        }

        const auto taskRank = this->mCurrentTask->getRank();
        const auto taskPeriod = this->mCurrentTask->getPeriod();

        // update cursor's rank
        this->mCursorTask.setRank(taskRank);

        // execute the task
        this->mCurrentTask->run();

        // Check if task is still linked after execution
        if (this->mCurrentTask->isLinked()) {

            // the task is still in the list
            // update the task rank
            this->mCurrentTask->setRank(
                taskRank +
                taskPeriod
            );

            this->sortTask(*this->mCurrentTask);

        }
        else if (this->empty()) {
            this->mCurrentTask = nullptr;
            mTimer.stop();
            return;
        }

        this->mCurrentTask = nullptr;
        mTimer.setDuration(this->getNextRank() - taskRank);
    }

}