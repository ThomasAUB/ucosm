#include "rt_scheduler.hpp"

namespace ucosm {

    bool RTScheduler::setTimer(ITimer& inTimer) {

        if (mTimer || !inTimer.setTask(*this)) {
            // scheduler already has a timer
            // or timer is not free
            return false;
        }

        mTimer = &inTimer;
        return true;
    }

    bool RTScheduler::addTask(IPeriodicTask& inTask) {
        return this->addTask(inTask, 0);
    }

    bool RTScheduler::addTask(IPeriodicTask& inTask, IPeriodicTask::tick_t inDelay) {

        if (inTask.getPeriod() == 0 || !mTimer) {
            // Invalid period
            return false;
        }

        struct InterruptGuard {
            ITimer& timer;
            InterruptGuard(ITimer& t) :
                timer(t) {
                timer.disableInterruption();
            }
            ~InterruptGuard() {
                timer.enableInterruption();
            }
        } guard(*mTimer);

        if (!base_t::addTask(inTask)) {
            return false;
        }

        if (inDelay) {
            inTask.setRank(this->mCursorTask.getRank() + inDelay);
            this->sortTask(inTask);
        }

        if (!mTimer->isRunning()) {
            mTimer->setDuration(0);
            mTimer->start();
        }

        return true;
    }

    void RTScheduler::run() {

        this->mCurrentTask = this->getNextTask();

        if (!this->mCurrentTask) {
            // no task to execute
            // scheduler is empty
            mTimer->stop();
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
            mTimer->stop();
            return;
        }

        this->mCurrentTask = nullptr;

        mTimer->setDuration(
            this->getNextRank() -
            taskRank
        );
    }

    RTScheduler::~RTScheduler() {
        if (mTimer) {
            mTimer->stop();
        }
    }

}