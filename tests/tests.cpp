#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"


#include "ucosm/scheduler.hpp"
#include "iperiodic_task.hpp"


#include "cfs/icfs_task.hpp"
#include "cfs/cfs_tick.hpp"

#include <chrono>


TEST_CASE("basic ucosm tests") {

    ucosm::periodic::IPeriodicTask::setTickFunction(
        +[] () {
            static auto start = std::chrono::steady_clock::now();
            auto end = std::chrono::steady_clock::now();
            return static_cast<ucosm::periodic::tick_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                );
        }
    );

    struct Task : ucosm::periodic::IPeriodicTask {

        void deinit() override {
            mIsDeinit = true;
        }

        void onPeriodElapsed() override {
            if (mCounter++ == count) {
                this->remove();
            }
        }

        uint8_t count = 0;
        uint8_t mCounter = 0;
        bool mIsDeinit = false;
    };

    ucosm::Scheduler<ucosm::periodic::tick_t, uint8_t> sched(ucosm::periodic::IPeriodicTask::getTick);

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

