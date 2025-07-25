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

#include <stdint.h>
#include "irt_timer.hpp"
#include "ucosm/core/ischeduler.hpp"
#include "ucosm/periodic/iperiodic_task.hpp"

namespace ucosm {

    /**
     * @brief Real-time scheduler.
     */
    struct RTScheduler : IScheduler<IPeriodicTask, ITask<uint8_t>> {

        using ITimer = IRTTimer<ITask<uint8_t>>;

        bool setTimer(ITimer& inTimer) {
            if (mTimer || !inTimer.setTask(*this)) {
                // scheduler already has a timer
                // or timer is not free
                return false;
            }

            mTimer = &inTimer;
            return true;
        }

        bool addTask(IPeriodicTask& inTask) override {
            return this->addTask(inTask, 0);
        }

        bool addTask(IPeriodicTask& inTask, IPeriodicTask::tick_t inDelay) {

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

        ~RTScheduler() {
            if (mTimer) {
                mTimer->stop();
                mTimer->removeTask();
            }
        }

    protected:

        void delay(uint32_t inDelay) {
            mTimer->setDuration(inDelay);
            mCounter += inDelay;
        }

        void run() override {

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


        uint32_t mCounter = 0;
        using base_t = IScheduler<IPeriodicTask, ITask<uint8_t>>;
        ITimer* mTimer = nullptr;
    };

}