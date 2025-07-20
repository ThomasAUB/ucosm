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
                timer.disable();
            }
            ~InterruptGuard() {
                timer.enable();
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
            mTimer->setDuration(inDelay);
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

        // check if the task to be executed hasn't been deleted since the timer has been programed
        const auto cursorRank = this->mCursorTask.getRank();
        const auto currentRank = this->mCurrentTask->getRank();

        const auto deltaTask = currentRank - cursorRank;
        const auto deltaTick = mCounter - cursorRank;

        if (deltaTick < deltaTask) {

            // task is not ready

            if (auto* next = this->getNextTask()) {
                delay(next->getRank() - mCounter);
            }
            else {
                // no other task to execute
                mTimer->stop();
            }

            this->mCurrentTask = nullptr;
            return;
        }

        // execute the task
        this->mCursorTask.setRank(currentRank);
        this->mCurrentTask->run();

        // Check if task is still linked after execution
        if (this->mCurrentTask->isLinked()) {

            // the task is still in the list
            // update the task rank
            this->mCurrentTask->setRank(
                currentRank +
                this->mCurrentTask->getPeriod()
            );

            this->sortTask(*this->mCurrentTask);
        }
        else if (this->empty()) {
            mTimer->stop();
            this->mCurrentTask = nullptr;
            return;
        }

        delay(this->getNextRank() - currentRank);
        this->mCurrentTask = nullptr;
    }

    void RTScheduler::delay(uint32_t inDelay) {
        mTimer->setDuration(inDelay);
        mCounter += inDelay;
    }

    RTScheduler::~RTScheduler() {
        if (mTimer) {
            mTimer->stop();
            mTimer->removeTask();
        }
    }

}