#include "tests.hpp"

#include "doctest.h"

#include "ucosm/periodic/periodic_scheduler.hpp"

#include <chrono>

#include <iostream>

auto getMS() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void periodicTaskTests() {

    struct Task : ucosm::IPeriodicTask {

        bool init() override {
            mTimer = getMS() - this->getPeriod();
            return true;
        }

        void deinit() override {
            mIsDeinit = true;
        }

        void run() override {

            // check that the period is right
            CHECK((getMS() - mTimer) == this->getPeriod());

            mTimer = getMS();

            if (mCounter++ == count) {
                this->removeTask();
            }
        }

        uint64_t mTimer = 0;
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


