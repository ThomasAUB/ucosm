#include "tests.hpp"

#include "doctest.h"

#include "cfs/cfs_scheduler.hpp"

#include <chrono>

void cfsTaskTests() {

    struct Task : ucosm::ICFSTask {

        void run() override {

            for (volatile uint32_t i = 0; i < mLength; i++) {}

            if (mCounter++ == count) {
                this->remove();
            }
        }

        void setLength(uint32_t io) { mLength = io; }

        uint8_t count = 0;
        uint8_t mCounter = 0;
        uint32_t mLength;
    };

    ucosm::CFSScheduler sched(
        +[] () {
            return static_cast<ucosm::ICFSTask::tick_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
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
    t1.setPriority(4);
    t1.setLength(1000);

    t2.count = 10;
    t2.setPriority(6);
    t2.setLength(1000);

    sched.addTask(t1);
    sched.addTask(t2);

    CHECK(sched.size() == 2);
    CHECK(!sched.empty());

    while (!sched.empty()) {
        sched.run();
    }

}