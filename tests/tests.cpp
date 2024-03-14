#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "ucosm/itask.hpp"
#include "ucosm/scheduler.hpp"

#include "periodic/iperiodic_task.hpp"
#include <chrono>

TEST_CASE("basic ucosm tests") {

    struct Task : ucosm::ITask<uint8_t> {

        void deinit() override {
            std::cout << "deinit" << std::endl;
        }

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

    {
        Task t22;
        sched.addTask(t22);
    }

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


    ucosm::setTickFunction(
        +[] () {
            static auto start = std::chrono::steady_clock::now();
            auto end = std::chrono::steady_clock::now();
            return static_cast<ucosm::tick_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        }
    );

    struct PTask : ucosm::IPeriodicTask {

        void periodElapsed() override {
            std::cout << "period " << this->period() << std::endl;
        }

    };


    ucosm::Scheduler<ucosm::tick_t, int> psched(
        +[] (ucosm::tick_t inTS) {
            return ucosm::getTick() >= inTS;
        }
    );

    PTask p;
    PTask p2;

    p.setPeriod(1000);
    p.setDelay(2000);

    p2.setPeriod(500);

    psched.addTask(p);
    psched.addTask(p2);

    uint32_t counter = 0;

    while (counter++ < 100000) {
        psched.run();
    }

}

