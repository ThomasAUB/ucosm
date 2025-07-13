#include "tests.hpp"
#include "doctest.h"

#include "ucosm/periodic/periodic_scheduler.hpp"

#include <iostream>
#include <iomanip>

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

        Task(int inID) : mID(inID) {}

        void deinit() override {
            mIsDeinit = true;
        }

        void run() override {

            auto currentTime = getMillis();

            if (mFirstExecution) {
                mFirstExecution = false;
            }
            else {
                double period = currentTime - mLastExecution;
                double error = 100 - ((double) this->getPeriod() / period) * 100;
                mAverageError += error;

                std::cout
                    << "Task " << mID << " "
                    << "| period =" << getPeriod() << "ms "
                    << "| actual =" << period << "ms "
                    << "| error =" << std::fixed << std::setprecision(1) << error << "%" << std::endl;

            }

            mLastExecution = currentTime;

            // simulate work
            waitFor_ms(5);

            if (mCounter++ == count) {
                std::cout << "Task " << mID << " completed" << std::endl;
                this->removeTask();
            }

        }

        int error() const {
            if (mCounter < 2) {
                return 0;
            }
            return mAverageError / (mCounter - 1);
        }

        uint8_t count = 0;
        bool mIsDeinit = false;

    private:
        double mAverageError = 0;
        bool mFirstExecution = true;
        uint32_t mLastExecution = 0;
        uint8_t mCounter = 0;
        int mID;
    };

    ucosm::PeriodicScheduler sched(getMillis);

    Task t1(1);
    Task t2(2);

    CHECK(sched.size() == 0);
    CHECK(sched.empty());

    {
        Task t3(3);
        sched.addTask(t3);
        CHECK(sched.size() == 1);
    }

    CHECK(sched.size() == 0);
    CHECK(sched.empty());

    t1.count = 6;
    t1.setPeriod(125);

    t2.count = 10;
    t2.setPeriod(50);

    sched.addTask(t1);
    sched.addTask(t2);

    CHECK(sched.size() == 2);
    CHECK(!sched.empty());
    CHECK(!t1.mIsDeinit);
    CHECK(!t2.mIsDeinit);

    std::cout << "=== Periodic Scheduler start ===" << std::endl;

    while (!sched.empty()) {
        sched.run();
    }

    CHECK(t1.error() == 0);
    CHECK(t2.error() == 0);
    CHECK(t1.mIsDeinit);
    CHECK(t2.mIsDeinit);

    std::cout << "=== Periodic Scheduler end ===" << std::endl;
}

void sortBenchmak() {

    struct Task : ucosm::IPeriodicTask {

        Task(uint32_t inPeriod = 5) : ucosm::IPeriodicTask(inPeriod) {}

        void run() override {

        }

    };

    ucosm::PeriodicScheduler sched(getMillis);

    constexpr uint16_t task_count = 512;
    Task taskArray[task_count];

    for (int i = 0; i < task_count; i++) {
        sched.addTask(taskArray[i]);
    }

    CHECK(sched.size() == task_count);

    Task longPeriodTask(500);
    sched.addTask(longPeriodTask);

    uint64_t durationSum = 0;

    for (uint32_t i = 0; i < 10'000; i++) {

        auto start = getMicros();
        sched.run();
        auto end = getMicros();

        durationSum += end - start;
    }

    // about 500 us in release and 1200 in debug
    std::cout << "relocation benchmark : " << durationSum << std::endl;

}