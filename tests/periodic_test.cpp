#include "tests.hpp"

#include "doctest.h"

#include "ucosm/periodic/periodic_scheduler.hpp"

#include <chrono>

#include <iostream>

static auto getMS() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void basicTest();

void timerOverflowTest();

void sortBenchmak();

void periodicTaskTests() {

    basicTest();

    timerOverflowTest();

    sortBenchmak();

}

void timerOverflowTest() {

    struct Task : ucosm::IPeriodicTask {

        void run() override {
            mRunCounter++;
        }

        uint32_t mRunCounter = 0;

    };

    static uint32_t sClock = 0;

    ucosm::PeriodicScheduler sched(
        +[] () {
            return sClock;
        }
    );

    Task t1;
    Task t2;
    Task t3;

    sched.addTask(t1);
    sched.addTask(t2);
    sched.addTask(t3);

    t1.setPeriod(0xFFFFFFFE);
    t2.setPeriod(0xFF);
    t3.setPeriod(0xFF);

    sched.setDelay(t1, 0x000000FF);
    sched.setDelay(t2, 0xFFFFFFFF - 10);
    sched.setDelay(t3, 0xFFFFFFFF - 1);

    sched.run();

    CHECK(t1.mRunCounter == 0);
    CHECK(t2.mRunCounter == 0);
    CHECK(t3.mRunCounter == 0);

    sClock = 256;

    sched.run();

    CHECK(t1.mRunCounter == 1);
    CHECK(t2.mRunCounter == 0);
    CHECK(t3.mRunCounter == 0);

    sClock = 0xFFFFFFFF - 9;

    sched.run();

    CHECK(t1.mRunCounter == 1);
    CHECK(t2.mRunCounter == 1);
    CHECK(t3.mRunCounter == 0);

    // scheduler got stuck for a while
    sClock = 5;

    sched.run();

    CHECK(t1.mRunCounter == 1);
    CHECK(t2.mRunCounter == 1);
    CHECK(t3.mRunCounter == 1);

    sClock = 244;

    sched.run();

    CHECK(t1.mRunCounter == 1);
    CHECK(t2.mRunCounter == 1);
    CHECK(t3.mRunCounter == 1);

    sClock = 245;

    sched.run();

    CHECK(t1.mRunCounter == 1);
    CHECK(t2.mRunCounter == 2);
    CHECK(t3.mRunCounter == 1);
}

void basicTest() {

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

void sortBenchmak() {

    struct Task : ucosm::IPeriodicTask {

        Task(uint32_t inPeriod = 5) : ucosm::IPeriodicTask(inPeriod) {}

        void run() override {

        }

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

    auto getMicros = [] () {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
            );
        };

    constexpr uint16_t task_count = 512;
    Task taskArray[task_count];

    for (int i = 0; i < task_count; i++) {
        sched.addTask(taskArray[i]);
    }

    CHECK(sched.size() == task_count);

    Task longPeriodTask(500);
    sched.addTask(longPeriodTask);

    uint64_t durationSum = 0;

    for (uint32_t i = 0; i < 10000; i++) {

        auto start = getMicros();
        sched.run();
        auto end = getMicros();

        durationSum += end - start;
    }

    // about 510 us in release and 1351 in debug
    std::cout << "relocation benchmark : " << durationSum << std::endl;

}