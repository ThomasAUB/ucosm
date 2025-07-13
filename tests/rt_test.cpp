#include "tests.hpp"

#include "doctest.h"

#include "ucosm/rt/rt_scheduler.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iomanip>

static uint32_t getMS() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

class Timer : public ucosm::RTScheduler::ITimer {
private:
    std::atomic<bool> mRunning { false };
    std::atomic<bool> mInterruptionsEnabled { true };
    std::atomic<uint32_t> mDuration { 0 };
    std::thread mTimerThread;
    std::atomic<bool> mStopRequested { false };

    // Absolute timer behavior: track when setDuration was called
    std::chrono::steady_clock::time_point mSchedulerStartTime;
    std::atomic<uint32_t> mCurrentRank { 0 };
    std::mutex mTimerMutex;
    std::condition_variable mTimerCV;
    std::atomic<bool> mDurationSet { false };

    void timerISR() {
        while (!mStopRequested.load()) {
            if (mRunning.load()) {
                std::unique_lock<std::mutex> lock(mTimerMutex);

                // Wait for duration to be set
                mTimerCV.wait(lock, [this] {
                    return mDurationSet.load() || mStopRequested.load() || !mRunning.load();
                    });

                if (mStopRequested.load() || !mRunning.load()) {
                    continue;
                }

                const uint32_t currentDuration = mDuration.load();
                mDurationSet.store(false);
                lock.unlock();

                // If duration is 0, fire immediately and reset time base
                if (currentDuration == 0) {
                    mSchedulerStartTime = std::chrono::steady_clock::now();
                    mCurrentRank.store(0);
                    if (mInterruptionsEnabled.load()) {
                        processIT();
                    }
                    continue;
                }

                // Update rank and calculate absolute fire time
                mCurrentRank.fetch_add(currentDuration);
                auto nextFireTime = mSchedulerStartTime + std::chrono::milliseconds(mCurrentRank.load());

                // Sleep until the exact fire time
                std::this_thread::sleep_until(nextFireTime);

                // Fire the timer interrupt if still running and interruptions enabled
                if (mRunning.load() && mInterruptionsEnabled.load()) {
                    processIT();
                }
            }
            else {
                // Timer is stopped, just wait
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

public:

    Timer() {
        mTimerThread = std::thread(&Timer::timerISR, this);
    }

    ~Timer() {
        mStopRequested.store(true);
        if (mTimerThread.joinable()) {
            mTimerThread.join();
        }
    }

    void start() override {
        if (!mRunning.load()) {
            mRunning.store(true);
        }
    }

    void stop() override {
        mRunning.store(false);
    }

    bool isRunning() const override {
        return mRunning.load();
    }

    void setDuration(uint32_t inDuration) override {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        mDuration.store(inDuration);
        mDurationSet.store(true);
        mTimerCV.notify_one();
    }

    void disableInterruption() override {
        mInterruptionsEnabled.store(false);
    }

    void enableInterruption() override {
        mInterruptionsEnabled.store(true);
    }
};

void rtTaskTests() {

    struct RTTask : ucosm::IPeriodicTask {

        RTTask(int id, uint32_t inPeriod) :
            ucosm::IPeriodicTask(inPeriod), mID(id) {}

        void run() override {

            auto currentTime = getMS();

            if (mFirstExecution) {
                //std::cout << "Task " << mID << " first execution" << std::endl;
                mFirstExecution = false;
            }
            else {

                auto timeSinceLastRun = currentTime - mLastExecution;
                double accuracy = (double) timeSinceLastRun / (double) getPeriod() * 100.0;

                std::cout << "Task " << mID << " period=" << getPeriod() << "ms "
                    << "actual=" << timeSinceLastRun << "ms "
                    << "accuracy=" << std::fixed << std::setprecision(1) << accuracy << "%" << std::endl;

                mAverageAccuracy += accuracy;
            }

            mLastExecution = currentTime;

            // Simulate work
            auto start = getMS();
            while (getMS() - start < 10);

            if (mCounter++ == 5) {
                std::cout << "Task " << mID << " completed, removing itself" << std::endl;
                this->removeTask();
            }
        }

        uint8_t accuracy() const {
            if (mCounter < 2) {
                return 0;
            }
            return mAverageAccuracy / (mCounter - 1);
        }

    private:

        double mAverageAccuracy = 0;
        bool mFirstExecution = true;
        uint32_t mLastExecution;
        int mCounter = 0;
        int mID;
    };

    RTTask task1(1, 100);
    RTTask task2(2, 250);

    Timer tim;

    ucosm::RTScheduler sched(tim);

    std::cout << "=== RT Scheduler Test Starting ===" << std::endl;

    sched.addTask(task1);
    sched.addTask(task2);

    CHECK(tim.isRunning());

    const auto startTimeout = getMS();
    while (tim.isRunning() && (getMS() - startTimeout) < 20'000);

    CHECK(!tim.isRunning());
    CHECK(task1.accuracy() == 100);
    CHECK(task2.accuracy() == 100);

    std::cout << "Task 1 accuracy: " << (int) task1.accuracy() << "%" << std::endl;
    std::cout << "Task 2 accuracy: " << (int) task2.accuracy() << "%" << std::endl;

    std::cout << "=== RT Scheduler Test Completed ===" << std::endl;
}