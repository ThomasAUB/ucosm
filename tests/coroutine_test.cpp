#include "tests.hpp"
#include "doctest.h"

#include "ucosm/coroutine/icoroutine.hpp"
#include "ucosm/periodic/periodic_scheduler.hpp"

#include <iostream>

TEST_CASE("Coroutine task test") {

    struct Coroutine : ucosm::ICoroutine {

        int mCounter = 0;

        void run() override {

            UCOSM_START;

            mCounter = 0;
            std::cout << "start" << std::endl;

            UCOSM_YIELD;

            std::cout << "yielded" << std::endl;

            UCOSM_WAIT_UNTIL(mCounter++ == 5, 100);

            std::cout << "see you in 5 sec" << std::endl;

            UCOSM_WAIT(5000);

            std::cout << "comme back !" << std::endl;

            std::cout << "see you in 1 sec" << std::endl;

            UCOSM_WAIT(1000);

            std::cout << "comme back !" << std::endl;

            std::cout << "ending task" << std::endl;

            UCOSM_END;
        }

    };

    ucosm::PeriodicScheduler sched(getMillis);

    Coroutine t;

    sched.addTask(t);

    while (!sched.empty()) {
        sched.run();
    }
}