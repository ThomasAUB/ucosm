#include "tests.hpp"
#include "doctest.h"

#include "ucosm/rt/rt_scheduler.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <cmath>

class Timer : public ucosm::RTScheduler::ITimer {

    std::atomic<bool> mRunning { false };
    std::atomic<bool> mShutdown { false };
    std::atomic<bool> mEnabled { true };
    std::chrono::steady_clock::time_point mTimePoint;
    std::thread mTimerThread;

    void timerISR() {
        while (!mShutdown.load()) {
            if (mRunning.load() && mEnabled.load()) {
                std::this_thread::sleep_until(mTimePoint);
                mTimePoint = std::chrono::steady_clock::now();
                run();
            }
            else {
                // Small delay to prevent busy waiting when not running
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

public:

    Timer() {
        mTimerThread = std::thread(&Timer::timerISR, this);
    }

    ~Timer() {
        shutdown();
    }

    void start() override {
        mTimePoint = std::chrono::steady_clock::now();
        mRunning.store(true);
    }

    void stop() override {
        mRunning.store(false);
    }

    bool isRunning() const override {
        return mRunning.load();
    }

    void setDuration(uint32_t inDuration) override {
        mTimePoint += std::chrono::milliseconds(inDuration);
    }

    void disable() override {
        mEnabled.store(false);
    }

    void enable() override {
        mEnabled.store(true);
    }

    void shutdown() {
        // Signal shutdown first
        mShutdown.store(true);
        mRunning.store(false);

        // Wait for thread to finish
        if (mTimerThread.joinable()) {
            mTimerThread.join();
        }
    }

};

void rtTaskTests() {

    struct RTTask : ucosm::IPeriodicTask {

        RTTask(int id, uint32_t inPeriod) :
            ucosm::IPeriodicTask(inPeriod), mID(id) {}

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

            // Simulate work
            waitFor_ms(10);

            if (mCounter++ == 5) {
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

    private:

        double mAverageError = 0;
        bool mFirstExecution = true;
        uint32_t mLastExecution;
        int mCounter = 0;
        int mID;
    };

    std::cout << "=== RT Scheduler start ===" << std::endl;

    //////////////////////////////

    Timer tim;
    ucosm::RTScheduler sched;
    sched.setTimer(tim);
    RTTask task1(1, 1000);
    RTTask task2(2, 2500);
    sched.addTask(task1);
    sched.addTask(task2);

    //////////////////////////////

    Timer tim2;
    ucosm::RTScheduler sched2;
    sched2.setTimer(tim2);
    RTTask task3(3, 50);
    sched2.addTask(task3);

    //////////////////////////////

    CHECK(tim.isRunning());
    CHECK(tim2.isRunning());



    {
        const auto startTimeout = getMillis();
        while ((tim.isRunning() || tim2.isRunning()) && (getMillis() - startTimeout) < 30'000);
    }

    CHECK(!tim.isRunning());
    CHECK(!tim2.isRunning());

    int t1AbsError = std::abs(task1.error());
    int t2AbsError = std::abs(task2.error());
    int t3AbsError = std::abs(task3.error());

    // check that the error is <= to 1 %
    CHECK(t1AbsError <= 1);
    CHECK(t2AbsError <= 1);
    CHECK(t3AbsError == 0);

    std::cout << "Task 1 average error: " << (int) task1.error() << "%" << std::endl;
    std::cout << "Task 2 average error: " << (int) task2.error() << "%" << std::endl;
    std::cout << "Task 3 average error: " << (int) task3.error() << "%" << std::endl;

    tim.shutdown();
    tim2.shutdown();

    std::cout << "\n=== RT Scheduler end ===\n" << std::endl;
}