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

#include <atomic>
#include <stdint.h>
#include "ucosm/core/deadline.hpp"
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
                inTask.setRank(makeDeadline(this->mCursorTask.getRank(), inDelay));
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
            mCounter.fetch_add(inDelay, std::memory_order_relaxed);
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
            const auto currentRank = this->mCurrentTask->getRank();
            const auto counter = mCounter.load(std::memory_order_acquire);

            if (!isDeadlineDue(this->mCursorTask.getRank(), currentRank, counter)) {

                // task is not ready

                if (auto* next = this->getNextTask()) {
                    delay(getDeadlineDelay(counter, next->getRank()));
                }
                else {
                    // no other task to execute
                    mTimer->stop();
                }

                this->mCurrentTask = nullptr;
                return;
            }

            // execute the task. Re-arm using the task's previous rank as
            // the time base (drift-free semantics: next deadline is anchored
            // to the one we just fired, not to the current tick).
            this->runAndRearm(*this->mCurrentTask, currentRank);

            if (this->mCurrentTask->isLinked()) {
                // task still scheduled: program the next interrupt
                delay(getDeadlineDelay(currentRank, this->getNextRank()));
            }
            else if (this->empty()) {
                mTimer->stop();
            }
            else {
                // task removed itself but others remain
                delay(getDeadlineDelay(currentRank, this->getNextRank()));
            }

            this->mCurrentTask = nullptr;
        }


        std::atomic<uint32_t> mCounter { 0 };
        using base_t = IScheduler<IPeriodicTask, ITask<uint8_t>>;
        ITimer* mTimer = nullptr;
    };

}