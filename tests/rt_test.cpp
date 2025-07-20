#include "tests.hpp"
#include "doctest.h"

#include "ucosm/rt/rt_scheduler.hpp"
#include "ucosm/rt/rt_inter_task.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <cmath>

#include "doctest.h"

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
    uint32_t mCounter;
    std::thread mTimerThread;


    void timerISR() {
#ifdef _WIN32
        // Set high priority for this thread on Windows
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif

        while (!mShutdown.load(std::memory_order_acquire)) {
            if (mRunning.load(std::memory_order_acquire) && mEnabled.load(std::memory_order_acquire)) {
                std::this_thread::sleep_until(mNextWakeup);
                mCounter = getMillis();
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

TEST_CASE("RT task test") {

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
    RTTask task2(2, 225);
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

TEST_CASE("RT Message Queue") {
    using namespace ucosm;

    SUBCASE("Basic send/receive") {
        RTMessageQueue<int, 4> queue;

        // Test empty queue
        CHECK(queue.empty());
        CHECK(queue.size() == 0);

        int value;
        CHECK_FALSE(queue.tryReceive(value));

        // Test sending
        CHECK(queue.trySend(42));
        CHECK_FALSE(queue.empty());
        CHECK(queue.size() == 1);

        // Test receiving
        CHECK(queue.tryReceive(value));
        CHECK(value == 42);
        CHECK(queue.empty());
    }

    SUBCASE("Queue capacity") {
        RTMessageQueue<int, 4> queue;  // Changed from 3 to 4 (power of 2)

        // Fill queue (capacity is actually Size-1 = 3)
        CHECK(queue.trySend(1));
        CHECK(queue.trySend(2));
        CHECK(queue.trySend(3));
        CHECK(queue.size() == 3);

        // Queue should be full
        CHECK_FALSE(queue.trySend(4));

        // Receive one
        int value;
        CHECK(queue.tryReceive(value));
        CHECK(value == 1);

        // Should be able to send again
        CHECK(queue.trySend(4));
    }

    SUBCASE("FIFO order") {
        RTMessageQueue<int, 8> queue;

        // Send sequence
        for (int i = 1; i <= 5; ++i) {
            CHECK(queue.trySend(i));
        }

        // Receive in same order
        for (int i = 1; i <= 5; ++i) {
            int value;
            CHECK(queue.tryReceive(value));
            CHECK(value == i);
        }
    }
}

TEST_CASE("RT Shared Variable") {
    using namespace ucosm;

    SUBCASE("Basic operations") {
        RTSharedVariable<float> var(3.14f);

        CHECK(var.load() == 3.14f);

        var.store(2.71f);
        CHECK(var.load() == 2.71f);
    }

    SUBCASE("Version tracking") {
        RTSharedVariable<int> var(100);

        int value;
        uint32_t version1 = var.loadWithVersion(value);
        CHECK(value == 100);

        var.store(200);
        uint32_t version2 = var.loadWithVersion(value);
        CHECK(value == 200);
        CHECK(version2 != version1);

        // Same value, version should not change
        var.store(200);
        uint32_t version3 = var.loadWithVersion(value);
        CHECK(value == 200);
        CHECK(version3 == version2);
    }

    SUBCASE("Compare and swap") {
        RTSharedVariable<int> var(10);

        int expected = 10;
        CHECK(var.compareAndSwap(expected, 20));
        CHECK(var.load() == 20);
        CHECK(expected == 10); // Should not be modified on success

        expected = 10; // Wrong expected value
        CHECK_FALSE(var.compareAndSwap(expected, 30));
        CHECK(var.load() == 20); // Should not change
        CHECK(expected == 20); // Should be updated with current value
    }
}

TEST_CASE("RT Event Flags") {
    using namespace ucosm;

    SUBCASE("Basic flag operations") {
        RTEventFlags flags;

        CHECK(flags.getFlags() == 0);
        CHECK_FALSE(flags.testAny(0x01));
        CHECK_FALSE(flags.testAll(0x01));

        flags.setFlags(0x05); // Set bits 0 and 2
        CHECK(flags.getFlags() == 0x05);
        CHECK(flags.testAny(0x01)); // Test bit 0
        CHECK(flags.testAny(0x04)); // Test bit 2
        CHECK(flags.testAll(0x05)); // Test both bits
        CHECK_FALSE(flags.testAll(0x07)); // Test missing bit

        flags.clearFlags(0x01); // Clear bit 0
        CHECK(flags.getFlags() == 0x04);
        CHECK_FALSE(flags.testAny(0x01));
        CHECK(flags.testAny(0x04));
    }

    SUBCASE("Wait operations") {
        RTEventFlags flags;

        // Test timeout on empty flags
        CHECK_FALSE(flags.waitAny(0x01, 1)); // 1ms timeout
        CHECK_FALSE(flags.waitAll(0x03, 1));

        flags.setFlags(0x03);
        CHECK(flags.waitAny(0x01, 1));
        CHECK(flags.waitAll(0x03, 1));
        CHECK_FALSE(flags.waitAll(0x07, 1)); // Missing bit
    }
}

TEST_CASE("RT Communication Integration") {
    using namespace ucosm;

    SUBCASE("Message queue with complex types") {
        struct TestMessage {
            uint32_t id;
            float value;
            bool operator==(const TestMessage& other) const {
                return id == other.id && value == other.value;
            }
        };

        RTMessageQueue<TestMessage, 4> queue;

        TestMessage msg1 { 1, 3.14f };
        TestMessage msg2 { 2, 2.71f };

        CHECK(queue.trySend(msg1));
        CHECK(queue.trySend(msg2));

        TestMessage received;
        CHECK(queue.tryReceive(received));
        CHECK(received == msg1);

        CHECK(queue.tryReceive(received));
        CHECK(received == msg2);
    }

    SUBCASE("Event-driven communication") {
        RTMessageQueue<int, 4> queue;
        RTEventFlags events;

        const uint32_t DATA_READY = 0x01;

        // Producer
        CHECK(queue.trySend(42));
        events.setFlags(DATA_READY);

        // Consumer
        if (events.testAny(DATA_READY)) {
            int value;
            if (queue.tryReceive(value)) {
                CHECK(value == 42);
                if (queue.empty()) {
                    events.clearFlags(DATA_READY);
                }
            }
        }

        CHECK_FALSE(events.testAny(DATA_READY));
    }
}