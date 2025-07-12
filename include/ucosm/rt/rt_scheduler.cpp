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

        mISCriticalSection = true;

        if (!base_t::addTask(inTask)) {
            mISCriticalSection = false;
            return false;
        }

        mISCriticalSection = false;

        if (!mTimer.isRunning()) {
            mTimer.start();
        }

        return true;
    }

    void RTScheduler::run() {

        if (mISCriticalSection) {
            return;
        }

        if (this->empty()) {
            mTimer.stop();
            return;
        }

        // cursor is last
        if (&this->mCursorTask == &this->mTasks.back()) {
            this->mCurrentTask = static_cast<IPeriodicTask*>(&this->mTasks.front());
        }
        else {
            this->mCurrentTask = this->mCursorTask.next();
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
            this->mCurrentTask->setRank(taskRank + taskPeriod);
            this->sortTask(*this->mCurrentTask);

        }
        else if (this->empty()) {
            this->mCurrentTask = nullptr;
            mTimer.stop();
            return;
        }

        this->mCurrentTask = nullptr;

        // find the future task to reprogram the timer duration
        IPeriodicTask* futureTask;

        if (&this->mCursorTask == &this->mTasks.back()) {
            futureTask = static_cast<IPeriodicTask*>(&this->mTasks.front());
        }
        else {
            futureTask = this->mCursorTask.next();
        }

        mTimer.setDuration(futureTask->getRank() - taskRank);
    }

}