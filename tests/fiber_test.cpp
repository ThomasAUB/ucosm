#include "tests.hpp"
#include <iostream>
#include <chrono>
#include "doctest.h"

#include "ucosm/timer/ifiber.hpp"
#include "ucosm/timer/timer_scheduler.hpp"

void fiberTests() {

    struct Fiber : ucosm::IFiber<> {

        int mI;
        uint32_t mPeriod;

        Fiber(int i, uint32_t inPeriod) : mI(i), mPeriod(inPeriod) {}

        void runFiber() override {

            std::cout << "init " << mI << std::endl;

            yield();

            int i = 0;

            while (i++ < 10) {
                std::cout << "sleep for " << mPeriod << " in " << mI << std::endl;

                sleepFor(mPeriod);
            }

        }

    };

    Fiber f1(1, 100);
    Fiber f2(2, 1500);

    ucosm::TimerScheduler sched(
        +[] () {
            return static_cast<ucosm::IBasicFiber::tick_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
                );
        }
    );

    sched.addTask(f1);
    sched.addTask(f2);

    while (true) {
        sched.run();
    }

}