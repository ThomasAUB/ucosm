#pragma once

#include "itimer_task.hpp"

#define TINA_IMPLEMENTATION
#define TINA_NO_CRT
#include "tina.h"

namespace ucosm {

    struct IBasicFiber : ITimerTask {

        IBasicFiber(uint8_t* inStack, std::size_t inStackSize) {
            mCoroutine = tina_init(
                inStack,
                inStackSize,
                +[] (tina* c, void* instance) {
                    reinterpret_cast<IBasicFiber*>(instance)->runFiber();
                    return static_cast<void*>(nullptr);
                },
                this
            );
        }

        virtual void runFiber() = 0;

        void run() override {
            this->mPeriod = 0;
            tina_resume(mCoroutine, this);
            if (mCoroutine->completed) {
                this->removeTask();
            }
        }

        void yield() {
            tina_yield(mCoroutine, 0);
        }

        void sleepFor(uint32_t inSleepDuration) {
            this->mPeriod = inSleepDuration;
            yield();
        }

    private:
        tina* mCoroutine;
    };

    template<std::size_t stack_size = 64 * 1024>
    struct IFiber : IBasicFiber {
        IFiber() : IBasicFiber(mStack, stack_size) {}
    private:
        alignas(_TINA_MAX_ALIGN) uint8_t mStack[stack_size];
    };


}