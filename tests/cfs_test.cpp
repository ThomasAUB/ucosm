#include "tests.hpp"

#include "doctest.h"

#include "ucosm/cfs/cfs_scheduler.hpp"

void cfsTaskTests() {

    struct Task : ucosm::ICFSTask {

        void run() override {

            waitFor_ms(10);

            if (mCounter++ == count) {
                this->removeTask();
            }
        }

        void setLength(uint32_t io) { mLength = io; }

        uint8_t count = 0;
        uint8_t mCounter = 0;
        uint32_t mLength;
    };

    ucosm::CFSScheduler sched(getMicros);

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