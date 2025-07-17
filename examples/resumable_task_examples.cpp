/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Resumable Task Examples and Tests
 *
 * This file demonstrates safe usage of the IResumableTask macro system
 * and provides examples for common patterns.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "ucosm/periodic/ilong_task.hpp"
#include "ucosm/periodic/periodic_scheduler.hpp"
#include <iostream>
#include <chrono>

 // Mock time function for examples
uint32_t getCurrentTimeMs() {
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
        );
}

// Example 1: Basic sequence with delays
class SequenceTask : public ucosm::IResumableTask {
public:
    SequenceTask() : ucosm::IResumableTask(10) {} // 10ms base period

    void run() override {
        UCOSM_START
            std::cout << "Step 1: Starting sequence..." << std::endl;

        UCOSM_WAIT(1000)  // Wait 1 second
            std::cout << "Step 2: After 1 second delay" << std::endl;

        UCOSM_YIELD       // Yield to other tasks
            std::cout << "Step 3: After yield" << std::endl;

        UCOSM_DELAY_SEC(2)  // Wait 2 seconds using convenience macro
            std::cout << "Step 4: After 2 second delay" << std::endl;

        std::cout << "Sequence complete!" << std::endl;
        UCOSM_END
    }

protected:
    uint32_t getCurrentTime() const override {
        return getCurrentTimeMs();
    }
};

// Example 2: Looping task with state
class BlinkTask : public ucosm::IResumableTask {
private:
    bool ledState = false;
    int blinkCount = 0;

public:
    BlinkTask() : ucosm::IResumableTask(50) {} // 50ms base period

    void run() override {
        UCOSM_START
            std::cout << "LED Blink Task Started" << std::endl;

        while (blinkCount < 10) {
            // Turn LED on
            ledState = true;
            std::cout << "LED ON (blink " << (blinkCount + 1) << ")" << std::endl;

            UCOSM_DELAY_MS(500)  // On for 500ms

                // Turn LED off
                ledState = false;
            std::cout << "LED OFF" << std::endl;

            UCOSM_DELAY_MS(300)  // Off for 300ms

                blinkCount++;

            UCOSM_YIELD  // Allow other tasks to run
        }

        std::cout << "Blink sequence complete!" << std::endl;
        UCOSM_END
    }

protected:
    uint32_t getCurrentTime() const override {
        return getCurrentTimeMs();
    }
};

// Example 3: Conditional waiting
class SensorTask : public ucosm::IResumableTask {
private:
    int sensorValue = 0;
    int readingCount = 0;

    bool sensorReady() {
        // Simulate sensor readiness
        return (getCurrentTimeMs() % 1000) < 100;
    }

    int readSensor() {
        return getCurrentTimeMs() % 1024; // Simulate sensor reading
    }

public:
    SensorTask() : ucosm::IResumableTask(20) {} // 20ms base period

    void run() override {
        UCOSM_START
            std::cout << "Sensor Task Started" << std::endl;

        while (readingCount < 5) {
            std::cout << "Waiting for sensor to be ready..." << std::endl;

            // Wait until sensor is ready, with 3 second timeout
            // Wait for sensor to be ready (simplified check)
            if (!sensorReady()) {
                UCOSM_WAIT(100) // Wait 100ms and try again
            }

            if (sensorReady()) {
                sensorValue = readSensor();
                std::cout << "Sensor reading " << readingCount + 1
                    << ": " << sensorValue << std::endl;
                readingCount++;
            }
            else {
                std::cout << "Sensor timeout!" << std::endl;
                break;
            }

            UCOSM_DELAY_MS(500)  // Wait between readings
        }

        std::cout << "Sensor task complete!" << std::endl;
        UCOSM_END
    }

protected:
    uint32_t getCurrentTime() const override {
        return getCurrentTimeMs();
    }
};

// Example 4: State machine replacement
class StateMachineTask : public ucosm::IResumableTask {
private:
    enum State { INIT, CONNECTING, CONNECTED, SENDING, WAITING, ERROR };
    State currentState = INIT;
    int retryCount = 0;

    bool connectToServer() {
        return (getCurrentTimeMs() % 3000) < 100; // 1/30 chance
    }

    bool sendData() {
        return (getCurrentTimeMs() % 2000) < 200; // 1/10 chance
    }

public:
    StateMachineTask() : ucosm::IResumableTask(100) {} // 100ms base period

    void run() override {
        UCOSM_START
            currentState = INIT;
        std::cout << "State Machine: INIT" << std::endl;

        // Connection phase
        while (currentState != CONNECTED && retryCount < 3) {
            currentState = CONNECTING;
            std::cout << "State Machine: CONNECTING (attempt " << (retryCount + 1) << ")" << std::endl;

            UCOSM_DELAY_MS(1000)  // Connection attempt delay

                if (connectToServer()) {
                    currentState = CONNECTED;
                    std::cout << "State Machine: CONNECTED" << std::endl;
                }
                else {
                    retryCount++;
                    std::cout << "Connection failed, retrying..." << std::endl;
                    UCOSM_DELAY_MS(2000)  // Retry delay
                }
        }

        if (currentState != CONNECTED) {
            currentState = ERROR;
            std::cout << "State Machine: ERROR - Connection failed" << std::endl;
            UCOSM_END; // Early termination
        }

        // Data sending phase
        currentState = SENDING;
        std::cout << "State Machine: SENDING" << std::endl;

        UCOSM_DELAY_MS(500)  // Prepare data

            if (sendData()) {
                currentState = WAITING;
                std::cout << "State Machine: WAITING for response" << std::endl;

                UCOSM_DELAY_MS(2000)  // Wait for response

                    std::cout << "State Machine: Complete!" << std::endl;
            }
            else {
                currentState = ERROR;
                std::cout << "State Machine: ERROR - Send failed" << std::endl;
            }

        UCOSM_END
    }

protected:
    uint32_t getCurrentTime() const override {
        return getCurrentTimeMs();
    }
};

// Example 5: Debugging and introspection
class DebuggableTask : public ucosm::IResumableTask {
private:
    int step = 0;

public:
    DebuggableTask() : ucosm::IResumableTask(500) {}

    void run() override {
        // Show current state before execution
        std::cout << "Task state: " << ucosm::ResumableTaskDebugger::getStateName(*this)
            << " (line: " << getCurrentState() << ")" << std::endl;

        UCOSM_START
            step = 1;
        std::cout << "Debug Step 1" << std::endl;

        UCOSM_WAIT(1000)
            step = 2;
        std::cout << "Debug Step 2" << std::endl;

        UCOSM_WAIT(1000)
            step = 3;
        std::cout << "Debug Step 3" << std::endl;

        // Check if we want to restart
        if (step < 5) {
            step++;
            std::cout << "Restarting task..." << std::endl;
            resetState();
            UCOSM_LOOP
        }

        std::cout << "Debug task complete!" << std::endl;
        UCOSM_END
    }

protected:
    uint32_t getCurrentTime() const override {
        return getCurrentTimeMs();
    }
};

// Usage example function
void demonstrateResumableTasks() {
    std::cout << "=== Resumable Task Examples ===" << std::endl;

    // Create tasks
    SequenceTask sequence;
    BlinkTask blink;
    SensorTask sensor;
    StateMachineTask stateMachine;
    DebuggableTask debugTask;

    // Check initial states
    std::cout << "Initial states:" << std::endl;
    std::cout << "Sequence: " << ucosm::ResumableTaskDebugger::getStateName(sequence) << std::endl;
    std::cout << "Blink: " << ucosm::ResumableTaskDebugger::getStateName(blink) << std::endl;
    std::cout << "Sensor: " << ucosm::ResumableTaskDebugger::getStateName(sensor) << std::endl;

    // In a real application, you would add these to a scheduler:
    // ucosm::PeriodicScheduler scheduler(getCurrentTimeMs);
    // scheduler.addTask(sequence);
    // scheduler.addTask(blink);
    // scheduler.addTask(sensor);
    // scheduler.addTask(stateMachine);
    // scheduler.addTask(debugTask);

    // Manual execution for demonstration
    std::cout << "\n=== Manual Task Execution ===" << std::endl;

    // Run sequence task a few times to show progression
    for (int i = 0; i < 5; ++i) {
        std::cout << "\n--- Sequence Task Step " << (i + 1) << " ---" << std::endl;
        if (sequence.isLinked() || i == 0) {
            sequence.run();
        }
        if (ucosm::ResumableTaskDebugger::isEnded(sequence)) {
            std::cout << "Sequence task has ended." << std::endl;
            break;
        }
    }

    std::cout << "\n=== Safety Features ===" << std::endl;

    // Demonstrate state validation
    DebuggableTask testTask;
    std::cout << "Fresh task is valid: " << (testTask.isStateValid() ? "YES" : "NO") << std::endl;

    // Show state transitions
    testTask.run();
    std::cout << "After first run - State: " << ucosm::ResumableTaskDebugger::getStateName(testTask) << std::endl;
    std::cout << "Is waiting: " << (ucosm::ResumableTaskDebugger::isWaiting(testTask) ? "YES" : "NO") << std::endl;

    // Reset demonstration
    testTask.resetState();
    std::cout << "After reset - State: " << ucosm::ResumableTaskDebugger::getStateName(testTask) << std::endl;

    std::cout << "\n=== Examples Complete ===" << std::endl;
}
