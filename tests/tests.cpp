#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "ucosm/itask.hpp"
#include "ucosm/scheduler.hpp"


TEST_CASE("basic ucosm tests") {

    struct Task : ucosm::ITask<uint8_t> {
        void run() override {
            this->setRank(this->getRank() + 1);
            if (mCounter++ == count) {
                this->remove();
            }
        }
        uint8_t count = 0;
        uint8_t mCounter = 0;
    };

    static uint8_t readCount = 0;

    ucosm::Scheduler<uint8_t, uint8_t> sched(
        +[] (uint8_t) {
            return true;
        }
    );

    Task t1;
    Task t2;

    t1.count = 6;
    t2.count = 10;

    sched.addTask(t1);
    sched.addTask(t2);

    uint8_t schedCounter = 0;
    while (!sched.empty()) {
        sched.run();
        schedCounter++;
    }

    CHECK(1);


}

