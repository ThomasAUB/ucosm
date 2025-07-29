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

#include <iostream>
#include <chrono>
#include <string>
#include "ucosm/coroutine.hpp"

// Helper function to get current tick (milliseconds)
static ucosm::IPeriodicTask::tick_t getTick_ms() {
    return static_cast<ucosm::IPeriodicTask::tick_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

/**
 * @brief Simple counting coroutine that yields periodically.
 */
class CountingCoroutine : public ucosm::ICoroutine<> {
public:
    CountingCoroutine(const std::string& name, int max_count)
        : ucosm::ICoroutine<>()
        , mName(name)
        , mMaxCount(max_count) {}

    std::string_view name() override { 
        return mName; 
    }

protected:
    void execute() override {
        for (int i = 1; i <= mMaxCount; ++i) {
            std::cout << mName << ": Count " << i << "/" << mMaxCount << std::endl;
            
            if (i < mMaxCount) {
                yield(); // Let other coroutines run
            }
        }
        std::cout << mName << ": Finished!" << std::endl;
    }

private:
    std::string mName;
    int mMaxCount;
};

/**
 * @brief Producer coroutine that generates data with delays.
 */
class ProducerCoroutine : public ucosm::ICoroutine<> {
public:
    ProducerCoroutine() : ucosm::ICoroutine<>(), mData(0) {}

    std::string_view name() override { return "Producer"; }

    int getData() const { return mData; }
    bool hasData() const { return mData > 0; }
    void consumeData() { mData = 0; }

protected:
    void execute() override {
        for (int i = 1; i <= 5; ++i) {
            std::cout << "Producer: Generating data " << i << std::endl;
            mData = i * 10; // Generate some data
            
            std::cout << "Producer: Waiting 500ms..." << std::endl;
            waitFor(500); // Wait 500ms
            
            std::cout << "Producer: Data ready: " << mData << std::endl;
            
            // Wait until data is consumed
            waitUntil([this]() { return mData == 0; }, 100);
            
            std::cout << "Producer: Data was consumed" << std::endl;
        }
        std::cout << "Producer: Finished!" << std::endl;
    }

private:
    int mData;
};

/**
 * @brief Consumer coroutine that processes data from producer.
 */
class ConsumerCoroutine : public ucosm::ICoroutine<> {
public:
    ConsumerCoroutine(ProducerCoroutine& producer) 
        : ucosm::ICoroutine<>()
        , mProducer(producer) {}

    std::string_view name() override { return "Consumer"; }

protected:
    void execute() override {
        int processed = 0;
        
        while (processed < 5) {
            std::cout << "Consumer: Waiting for data..." << std::endl;
            
            // Wait until producer has data
            waitUntil([this]() { return mProducer.hasData(); }, 50);
            
            int data = mProducer.getData();
            std::cout << "Consumer: Processing data: " << data << std::endl;
            
            // Simulate processing time
            waitFor(200);
            
            std::cout << "Consumer: Processed data: " << data << std::endl;
            mProducer.consumeData();
            processed++;
        }
        std::cout << "Consumer: Finished!" << std::endl;
    }

private:
    ProducerCoroutine& mProducer;
};

/**
 * @brief State machine coroutine demonstrating complex control flow.
 */
class StateMachineCoroutine : public ucosm::ICoroutine<> {
public:
    enum class State {
        Init,
        Connecting,
        Connected,
        Sending,
        Waiting,
        Finished
    };

    StateMachineCoroutine() 
        : ucosm::ICoroutine<>()
        , mState(State::Init)
        , mRetries(0) {}

    std::string_view name() override { return "StateMachine"; }

protected:
    void execute() override {
        while (mState != State::Finished) {
            switch (mState) {
                case State::Init:
                    std::cout << "StateMachine: Initializing..." << std::endl;
                    waitFor(100);
                    mState = State::Connecting;
                    break;

                case State::Connecting:
                    std::cout << "StateMachine: Connecting... (attempt " 
                             << (mRetries + 1) << ")" << std::endl;
                    waitFor(300);
                    
                    // Simulate connection success/failure
                    if (mRetries < 2) {
                        std::cout << "StateMachine: Connection failed!" << std::endl;
                        mRetries++;
                        waitFor(500); // Wait before retry
                    } else {
                        std::cout << "StateMachine: Connected!" << std::endl;
                        mState = State::Connected;
                        mRetries = 0;
                    }
                    break;

                case State::Connected:
                    std::cout << "StateMachine: Connection established" << std::endl;
                    mState = State::Sending;
                    break;

                case State::Sending:
                    std::cout << "StateMachine: Sending data..." << std::endl;
                    waitFor(200);
                    mState = State::Waiting;
                    break;

                case State::Waiting:
                    std::cout << "StateMachine: Waiting for response..." << std::endl;
                    waitFor(400);
                    std::cout << "StateMachine: Response received!" << std::endl;
                    mState = State::Finished;
                    break;

                case State::Finished:
                    break;
            }
            
            if (mState != State::Finished) {
                yield(); // Give other coroutines a chance
            }
        }
        std::cout << "StateMachine: Operation completed!" << std::endl;
    }

private:
    State mState;
    int mRetries;
};

int main() {
    std::cout << "=== µCosm Coroutine Examples ===" << std::endl;
    
    // Create scheduler
    ucosm::CoroutineScheduler scheduler(getTick_ms);
    
    std::cout << "\n--- Example 1: Simple Counting Coroutines ---" << std::endl;
    
    // Create simple counting coroutines
    CountingCoroutine counter1("Counter1", 3);
    CountingCoroutine counter2("Counter2", 4);
    
    scheduler.addCoroutine(counter1);
    scheduler.addCoroutine(counter2);
    
    // Run until both finish
    while (scheduler.hasActiveCoroutines()) {
        scheduler.run();
    }
    
    std::cout << "\n--- Example 2: Producer-Consumer Pattern ---" << std::endl;
    
    // Create producer-consumer pair
    ProducerCoroutine producer;
    ConsumerCoroutine consumer(producer);
    
    scheduler.addCoroutine(producer);
    scheduler.addCoroutine(consumer);
    
    // Run until both finish
    while (scheduler.hasActiveCoroutines()) {
        scheduler.run();
    }
    
    std::cout << "\n--- Example 3: State Machine ---" << std::endl;
    
    // Create state machine
    StateMachineCoroutine stateMachine;
    scheduler.addCoroutine(stateMachine);
    
    // Run until finished
    while (scheduler.hasActiveCoroutines()) {
        scheduler.run();
    }
    
    std::cout << "\n=== All coroutines completed ===" << std::endl;
    
    return 0;
}