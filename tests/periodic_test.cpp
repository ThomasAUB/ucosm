#include "tests.hpp"

#include "doctest.h"

#include "periodic/periodic_scheduler.hpp"

#include <chrono>

#include <iostream>

void periodicTaskTests() {


    struct Task : ucosm::IPeriodicTask {

        void deinit() override {
            mIsDeinit = true;
        }

        void run() override {
            if (mCounter++ == count) {
                this->remove();
            }
        }

        uint8_t count = 0;
        uint8_t mCounter = 0;
        bool mIsDeinit = false;
    };

    ucosm::PeriodicScheduler sched(
        +[] () {
            return static_cast<ucosm::IPeriodicTask::tick_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
                );
        }
    );

    Task t1;
    Task t2;

    CHECK(sched.size() == 0);
    CHECK(sched.empty());

    {
        Task t3;
        sched.addTask(t3);
        CHECK(sched.size() == 1);
    }

    CHECK(sched.size() == 0);
    CHECK(sched.empty());

    t1.count = 6;
    t1.setPeriod(100);

    t2.count = 10;
    t2.setPeriod(50);

    sched.addTask(t1);
    sched.addTask(t2);

    CHECK(sched.size() == 2);
    CHECK(!sched.empty());
    CHECK(!t1.mIsDeinit);
    CHECK(!t2.mIsDeinit);

    while (!sched.empty()) {
        sched.run();
    }

    CHECK(t1.mIsDeinit);
    CHECK(t2.mIsDeinit);

}


