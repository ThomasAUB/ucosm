#include "tests.hpp"
#include "doctest.h"
#include <iostream>
#include <string_view>
#include "ucosm/resumable/ithread.h"

ucosm::ThreadScheduler* scheduler;

TEST_CASE("Thread test") {

    struct Thread : ucosm::IThread {

        std::string_view mName;
        uint32_t mDuration;

        Thread(std::string_view inName, uint32_t inDuration) :
            mName(inName),
            mDuration(inDuration) {}

        void run() override {

            int mStepCount = 0;

            while (true) {
                std::cout << mName << " " << mStepCount++ << " stack : " << this->mStackUsage << std::endl;
                scheduler->sleepFor(mDuration);
            }

        }

    };

    ucosm::ThreadScheduler ts(getMillis);
    scheduler = &ts;

    Thread task1("task 1", 1000);
    Thread task2("task 2", 3000);

    ts.addTask(task1);
    ts.addTask(task2);

    const auto start = getMillis();
    while (getMillis() - start < 10000) {  // Reduced from 100 to 10
        ts.run();
    }

}