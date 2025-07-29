/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2024 Thomas AUBERT                                                *
 *                                                                                 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy    *
 * of this software and associated documentation files (the "Software"), to deal   *
 * in the Software without restriction, including without limitation the rights    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is           *
 * furnished to do so, subject to the following conditions:                        *
 *                                                                                 *
 * The above copyright notice and this permission notice shall be included in all  *
 * copies or substantial portions of the Software.                                 *
 *                                                                                 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 * SOFTWARE.                                                                       *
 *                                                                                 *
 * github : https://github.com/ThomasAUB/ucosm                                     *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "tests.hpp"
#include "ucosm/coroutine.hpp"
#include <vector>
#include <string>

namespace {

    // Mock tick function for testing
    static ucosm::IPeriodicTask::tick_t g_tick = 0;
    ucosm::IPeriodicTask::tick_t getMockTick() {
        return g_tick;
    }
    
    void advanceTick(ucosm::IPeriodicTask::tick_t amount = 1) {
        g_tick += amount;
    }
    
    void resetTick() {
        g_tick = 0;
    }

    // Test coroutine that records execution steps
    class TestCoroutine : public ucosm::ICoroutine<> {
    public:
        TestCoroutine(std::vector<std::string>& log, const std::string& name)
            : ucosm::ICoroutine<>()
            , mLog(log)
            , mName(name) {}

        std::string_view name() override { return mName; }

    protected:
        void execute() override {
            mLog.push_back(mName + ": Start");
            
            mLog.push_back(mName + ": Step 1");
            yield();
            
            mLog.push_back(mName + ": Step 2");
            yield();
            
            mLog.push_back(mName + ": Step 3");
            
            mLog.push_back(mName + ": End");
        }

    private:
        std::vector<std::string>& mLog;
        std::string mName;
    };

    // Test coroutine with waiting
    class WaitingCoroutine : public ucosm::ICoroutine<> {
    public:
        WaitingCoroutine(std::vector<std::string>& log, const std::string& name)
            : ucosm::ICoroutine<>()
            , mLog(log)
            , mName(name) {}

        std::string_view name() override { return mName; }

    protected:
        void execute() override {
            mLog.push_back(mName + ": Start");
            
            mLog.push_back(mName + ": Before wait");
            waitFor(10); // Wait 10 ticks
            mLog.push_back(mName + ": After wait");
            
            mLog.push_back(mName + ": End");
        }

    private:
        std::vector<std::string>& mLog;
        std::string mName;
    };

    // Test coroutine with conditional waiting
    class ConditionalWaitCoroutine : public ucosm::ICoroutine<> {
    public:
        ConditionalWaitCoroutine(std::vector<std::string>& log, 
                               const std::string& name,
                               bool& condition)
            : ucosm::ICoroutine<>()
            , mLog(log)
            , mName(name)
            , mCondition(condition) {}

        std::string_view name() override { return mName; }

    protected:
        void execute() override {
            mLog.push_back(mName + ": Start");
            
            mLog.push_back(mName + ": Waiting for condition");
            waitUntil([this]() { return mCondition; }, 2);
            mLog.push_back(mName + ": Condition met");
            
            mLog.push_back(mName + ": End");
        }

    private:
        std::vector<std::string>& mLog;
        std::string mName;
        bool& mCondition;
    };

} // anonymous namespace

TEST_CASE("Coroutine - Basic Context Creation and State") {
    resetTick();
    std::vector<std::string> log;
    
    TestCoroutine coroutine(log, "Test");
    
    // Check initial state
    CHECK(coroutine.getState() == ucosm::CoroutineState::Created);
    CHECK(!coroutine.isFinished());
}

TEST_CASE("Coroutine - Basic Yield Behavior") {
    resetTick();
    std::vector<std::string> log;
    
    ucosm::CoroutineScheduler scheduler(getMockTick);
    TestCoroutine coroutine1(log, "Coro1");
    TestCoroutine coroutine2(log, "Coro2");
    
    scheduler.addCoroutine(coroutine1);
    scheduler.addCoroutine(coroutine2);
    
    // Run scheduler until all coroutines finish
    int max_iterations = 20; // Prevent infinite loop
    while (scheduler.hasActiveCoroutines() && max_iterations-- > 0) {
        scheduler.run();
        advanceTick();
    }
    
    // Check that both coroutines executed their steps
    CHECK(log.size() >= 8); // Each coroutine should have 4 log entries
    
    // Check that coroutines are finished
    CHECK(coroutine1.isFinished());
    CHECK(coroutine2.isFinished());
}

TEST_CASE("Coroutine - Wait For Ticks") {
    resetTick();
    std::vector<std::string> log;
    
    ucosm::CoroutineScheduler scheduler(getMockTick);
    WaitingCoroutine coroutine(log, "Waiter");
    
    scheduler.addCoroutine(coroutine);
    
    // Run a few iterations - coroutine should not complete immediately
    for (int i = 0; i < 5; ++i) {
        scheduler.run();
        advanceTick();
    }
    
    // Should still be waiting
    CHECK(!coroutine.isFinished());
    CHECK(log.size() == 2); // Should have "Start" and "Before wait"
    
    // Advance time to complete the wait
    advanceTick(10);
    
    // Run scheduler again
    scheduler.run();
    
    // Now it should complete
    CHECK(log.size() == 4); // Should have all log entries
    
    // Run once more to let it finish
    scheduler.run();
    CHECK(coroutine.isFinished());
}

TEST_CASE("Coroutine - Conditional Wait") {
    resetTick();
    std::vector<std::string> log;
    bool condition = false;
    
    ucosm::CoroutineScheduler scheduler(getMockTick);
    ConditionalWaitCoroutine coroutine(log, "ConditionalWaiter", condition);
    
    scheduler.addCoroutine(coroutine);
    
    // Run several iterations while condition is false
    for (int i = 0; i < 10; ++i) {
        scheduler.run();
        advanceTick(2); // Advance by check period
    }
    
    // Should still be waiting
    CHECK(!coroutine.isFinished());
    CHECK(log.size() == 2); // Should have "Start" and "Waiting for condition"
    
    // Set condition to true
    condition = true;
    
    // Run scheduler again
    scheduler.run();
    advanceTick(2);
    scheduler.run();
    
    // Now it should complete
    CHECK(log.size() >= 3); // Should have condition met message
    CHECK(coroutine.isFinished());
}

TEST_CASE("Coroutine - Multiple Coroutines Interleaving") {
    resetTick();
    std::vector<std::string> log;
    
    ucosm::CoroutineScheduler scheduler(getMockTick);
    TestCoroutine coroutine1(log, "First");
    TestCoroutine coroutine2(log, "Second");
    TestCoroutine coroutine3(log, "Third");
    
    scheduler.addCoroutine(coroutine1);
    scheduler.addCoroutine(coroutine2);
    scheduler.addCoroutine(coroutine3);
    
    // Run scheduler
    int max_iterations = 30;
    while (scheduler.hasActiveCoroutines() && max_iterations-- > 0) {
        scheduler.run();
        advanceTick();
    }
    
    // All coroutines should finish
    CHECK(coroutine1.isFinished());
    CHECK(coroutine2.isFinished());
    CHECK(coroutine3.isFinished());
    
    // Should have interleaved execution
    CHECK(log.size() == 12); // 3 coroutines × 4 log entries each
    
    // Check that coroutines started in order but interleaved
    CHECK(log[0] == "First: Start");
    CHECK(log[1] == "Second: Start");
    CHECK(log[2] == "Third: Start");
}

TEST_CASE("Coroutine - Scheduler Tick Tracking") {
    resetTick();
    
    ucosm::CoroutineScheduler scheduler(getMockTick);
    
    CHECK(scheduler.getCurrentTick() == 0);
    
    scheduler.run();
    CHECK(scheduler.getCurrentTick() == 0);
    
    advanceTick(5);
    scheduler.run();
    CHECK(scheduler.getCurrentTick() == 5);
    
    advanceTick(10);
    scheduler.run();
    CHECK(scheduler.getCurrentTick() == 15);
}

TEST_CASE("Coroutine - Stack Size Template Parameter") {
    resetTick();
    std::vector<std::string> log;
    
    // Test different stack sizes
    class SmallStackCoroutine : public ucosm::ICoroutine<8192> {
    public:
        SmallStackCoroutine(std::vector<std::string>& log) 
            : ucosm::ICoroutine<8192>(), mLog(log) {}
        
        std::string_view name() override { return "SmallStack"; }
        
    protected:
        void execute() override {
            mLog.push_back("SmallStack executed");
        }
        
    private:
        std::vector<std::string>& mLog;
    };
    
    class LargeStackCoroutine : public ucosm::ICoroutine<16384> {
    public:
        LargeStackCoroutine(std::vector<std::string>& log) 
            : ucosm::ICoroutine<16384>(), mLog(log) {}
        
        std::string_view name() override { return "LargeStack"; }
        
    protected:
        void execute() override {
            mLog.push_back("LargeStack executed");
        }
        
    private:
        std::vector<std::string>& mLog;
    };
    
    ucosm::CoroutineScheduler scheduler(getMockTick);
    SmallStackCoroutine smallStack(log);
    LargeStackCoroutine largeStack(log);
    
    scheduler.addCoroutine(smallStack);
    scheduler.addCoroutine(largeStack);
    
    // Run until complete
    while (scheduler.hasActiveCoroutines()) {
        scheduler.run();
        advanceTick();
    }
    
    CHECK(log.size() == 2);
    CHECK(smallStack.isFinished());
    CHECK(largeStack.isFinished());
}