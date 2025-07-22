#pragma once


#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ucosm/periodic/iperiodic_task.hpp"
#include "ucosm/periodic/periodic_scheduler.hpp"

namespace ucosm {

    struct IThread : IPeriodicTask {

        volatile uint8_t* mStackStart = nullptr;
        volatile uint8_t* mStackEnd = nullptr;
        std::size_t mStackUsage = 0;
        bool hasYielded = false;
        uint8_t mStack[1024];

        jmp_buf mJumpBuffer;
    };


    struct ThreadScheduler : IScheduler<IThread, ITask<int8_t>> {

        uint32_t(*mGetTick)();

        ThreadScheduler(uint32_t(*getTick)()) : mGetTick(getTick) {}

        void sleepFor(uint32_t inSleepDuration) {

            if (!sCurrent) {
                return;
            }

            sCurrent->hasYielded = true;

            sCurrent->setRank(mGetTick() + inSleepDuration);

            this->sortTask(*sCurrent);

            volatile uint8_t stackEnd = 0;

            sCurrent->mStackEnd = &stackEnd;

            sCurrent->mStackUsage =
                static_cast<size_t>(
                    sCurrent->mStackStart -
                    sCurrent->mStackEnd + 1
                    );

            memcpy(
                sCurrent->mStack,
                (const void*) sCurrent->mStackEnd,
                sCurrent->mStackUsage);

            if (setjmp(sCurrent->mJumpBuffer) == 0) {
                longjmp(mJumpBuffer, 1);
            }
            else {
                memcpy(
                    (void*) sCurrent->mStackEnd,
                    (const void*) sCurrent->mStack,
                    sCurrent->mStackUsage
                );
            }
        }

        void yield() {
            sleepFor(1);
        }

        void run() override {

            const auto tick = mGetTick();

            sCurrent = this->getNextTask();

            if (sCurrent) {

                const auto cursorRank = this->mCursorTask.getRank();
                const auto deltaTask = sCurrent->getRank() - cursorRank;
                const auto deltaTick = tick - cursorRank;

                if (deltaTick < deltaTask) {
                    // task is not ready
                    sCurrent = nullptr;
                }

            }

            if (!sCurrent) {

                if (this->mIdleTask) {
                    this->mIdleTask();
                }

                return;
            }

            // Set up the scheduler's jump point for yield() calls

            if (setjmp(mJumpBuffer) == 0) {

                if (sCurrent->hasYielded) {

                    longjmp(sCurrent->mJumpBuffer, 1);

                }
                else {

                    volatile uint8_t stackStart = 0;

                    sCurrent->mStackStart = &stackStart;

                    this->mCursorTask.setRank(sCurrent->getRank());
                    sCurrent->run();

                }

                sCurrent->removeTask();

            }

            sCurrent = nullptr;
        }

        jmp_buf mJumpBuffer;
        static inline IThread* sCurrent = nullptr;
    };

}