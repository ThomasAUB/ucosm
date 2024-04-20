#include "tests.hpp"

#include "doctest.h"

#include "ucosm/scheduler.hpp"
#include "iperiodic_task.hpp"

#include <chrono>

void periodicTaskTests() {

    ucosm::IPeriodicTask::setTickFunction(
        +[] () {
            static auto start = std::chrono::steady_clock::now();
            auto end = std::chrono::steady_clock::now();
            return static_cast<ucosm::tick_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                );
        }
    );

    struct Task : ucosm::IPeriodicTask {

        void deinit() override {
            mIsDeinit = true;
        }

        void periodicRun() override {
            if (mCounter++ == count) {
                this->remove();
            }
        }

        uint8_t count = 0;
        uint8_t mCounter = 0;
        bool mIsDeinit = false;
    };

    ucosm::Scheduler<ucosm::tick_t, uint8_t> sched(ucosm::IPeriodicTask::getTick);

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


