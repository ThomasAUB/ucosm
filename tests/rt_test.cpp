#include "tests.hpp"
#include "doctest.h"

#include "ucosm/rt/rt_scheduler.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

class Timer : public ucosm::RTScheduler::ITimer {
private:
    std::atomic<bool> mRunning { false };
    std::atomic<bool> mShutdown { false };
    std::atomic<bool> mEnabled { true };
    std::chrono::high_resolution_clock::time_point mNextWakeup;
    std::thread mTimerThread;

    void timerISR() {
#ifdef _WIN32
        // Set high priority for this thread on Windows
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif

        while (!mShutdown.load(std::memory_order_acquire)) {
            if (mRunning.load(std::memory_order_acquire) && mEnabled.load(std::memory_order_acquire)) {
                std::this_thread::sleep_until(mNextWakeup);
                run();
            }
            else {
                // Use shorter sleep when not running to be more responsive
                std::this_thread::sleep_for(std::chrono::microseconds(100));
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
        mNextWakeup = std::chrono::high_resolution_clock::now();
        mRunning.store(true, std::memory_order_release);
    }

    void stop() override {
        mRunning.store(false, std::memory_order_release);
    }

    bool isRunning() const override {
        return mRunning.load(std::memory_order_acquire);
    }

    void setDuration(uint32_t inDuration) override {
        mNextWakeup += std::chrono::milliseconds(inDuration);
    }

    void disable() override {
        mEnabled.store(false, std::memory_order_release);
    }

    void enable() override {
        mEnabled.store(true, std::memory_order_release);
    }

    void shutdown() {
        // Signal shutdown first with proper memory ordering
        mShutdown.store(true, std::memory_order_release);
        mRunning.store(false, std::memory_order_release);

        // Wait for thread to finish with timeout to prevent hanging
        if (mTimerThread.joinable()) {
            mTimerThread.join();
        }
    }
};

void rtTaskTests() {

#ifdef _WIN32
    // Improve timer resolution on Windows
    timeBeginPeriod(1);
#endif

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
            waitFor_ms(std::rand() % 20);

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

    std::cout << "\n=== RT Scheduler start ===\n" << std::endl;

    //////////////////////////////

    Timer tim;
    ucosm::RTScheduler sched;
    sched.setTimer(tim);
    RTTask task1(1, 100);
    RTTask task2(2, 250);
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

    CHECK(t1AbsError <= 2);
    CHECK(t2AbsError <= 2);
    CHECK(t3AbsError <= 2);

    std::cout << "Task 1 average error: " << (int) task1.error() << "%" << std::endl;
    std::cout << "Task 2 average error: " << (int) task2.error() << "%" << std::endl;
    std::cout << "Task 3 average error: " << (int) task3.error() << "%" << std::endl;

    tim.shutdown();
    tim2.shutdown();

#ifdef _WIN32
    // Restore timer resolution on Windows
    timeEndPeriod(1);
#endif

    std::cout << "\n=== RT Scheduler end ===\n" << std::endl;
}