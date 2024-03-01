#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "ucosm/itask.hpp"
#include "ucosm/scheduler.hpp"


TEST_CASE("basic ucosm tests") {

    static uint32_t sCounter = 0;

    struct MyTask : ucosm::ITask {

        void run() override {
            std::cout << "hello" << std::endl;
            this->setRank(sCounter + 10000000);
        }

        rank_t getRank() const override {
            return mRank - sCounter;
        }

    };

    struct MyOtherTask : ucosm::ITask {

        void run() override {
            std::cout << "coucou" << std::endl;
            this->setRank(sCounter + 1000000);
        }

        rank_t getRank() const override {
            return mRank - sCounter;
        }

    };


    ucosm::Scheduler sched;

    MyTask t1;
    MyOtherTask t2;

    sched.addTask(t1);
    sched.addTask(t2);

    while (1) {
        sched.run();
        sCounter++;
    }

    CHECK(1);


}

