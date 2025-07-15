/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Example: RT Inter-Task Communication
 *
 * This example demonstrates safe communication between RT tasks using the
 * lock-free communication primitives.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "ucosm/rt/rt_scheduler.hpp"
#include "ucosm/rt/rt_message_queue.hpp"
#include "ucosm/periodic/iperiodic_task.hpp"
#include <iostream>

 // Example message types
struct SensorData {
    uint32_t timestamp;
    float temperature;
    float humidity;
    uint16_t sensorId;
};

struct ControlCommand {
    uint8_t deviceId;
    uint8_t command;
    uint16_t value;
    uint32_t timestamp;
};

// Communication channels between tasks
ucosm::RTMessageQueue<SensorData, 16> sensorQueue;
ucosm::RTMessageQueue<ControlCommand, 8> controlQueue;
ucosm::RTSharedVariable<float> criticalTemperature(25.0f);
ucosm::RTEventFlags systemEvents;

// Event flag definitions
constexpr uint32_t EVENT_SENSOR_READY = 0x01;
constexpr uint32_t EVENT_CONTROL_NEEDED = 0x02;
constexpr uint32_t EVENT_EMERGENCY = 0x04;
constexpr uint32_t EVENT_CALIBRATION = 0x08;

/**
 * @brief High-frequency sensor task (100ms period)
 * Reads sensors and publishes data to other tasks
 */
class SensorTask : public ucosm::IPeriodicTask {
public:
    SensorTask() : ucosm::IPeriodicTask(100), mSensorId(1) {}

    void run() override {
        // Simulate sensor reading
        SensorData data;
        data.timestamp = getCurrentTime();
        data.temperature = 20.0f + (mCounter * 0.1f); // Simulated rising temp
        data.humidity = 45.0f + (mCounter % 10);
        data.sensorId = mSensorId;

        // Send sensor data (non-blocking)
        if (!sensorQueue.trySend(data)) {
            std::cout << "Sensor queue full! Data lost.\n";
        }

        // Update critical temperature if needed
        if (data.temperature > 30.0f) {
            criticalTemperature.store(data.temperature);
            systemEvents.setFlags(EVENT_EMERGENCY);
        }

        // Signal that new sensor data is available
        systemEvents.setFlags(EVENT_SENSOR_READY);

        std::cout << "Sensor[" << data.sensorId << "]: T=" << data.temperature
            << "°C, H=" << data.humidity << "% (t=" << data.timestamp << ")\n";

        mCounter++;
    }

private:
    uint32_t mCounter = 0;
    uint16_t mSensorId;

    uint32_t getCurrentTime() {
        // Simulate timestamp
        return mCounter * 100; // ms
    }
};

/**
 * @brief Control task (250ms period)
 * Processes sensor data and sends control commands
 */
class ControlTask : public ucosm::IPeriodicTask {
public:
    ControlTask() : ucosm::IPeriodicTask(250), mLastTempVersion(0) {}

    void run() override {
        // Check for emergency conditions
        if (systemEvents.testAny(EVENT_EMERGENCY)) {
            float criticalTemp = criticalTemperature.load();
            std::cout << "EMERGENCY: Critical temperature " << criticalTemp << "°C detected!\n";

            // Send emergency command
            ControlCommand cmd;
            cmd.deviceId = 1;
            cmd.command = 0xFF; // Emergency shutdown
            cmd.value = static_cast<uint16_t>(criticalTemp * 10);
            cmd.timestamp = getCurrentTime();

            if (!controlQueue.trySend(cmd)) {
                std::cout << "Control queue full! Emergency command lost!\n";
            }

            systemEvents.clearFlags(EVENT_EMERGENCY);
        }

        // Process normal sensor data
        SensorData sensorData;
        while (sensorQueue.tryReceive(sensorData)) {
            std::cout << "Control processing sensor " << sensorData.sensorId
                << " data: " << sensorData.temperature << "°C\n";

            // Decide if control action is needed
            if (sensorData.temperature > 25.0f) {
                ControlCommand cmd;
                cmd.deviceId = 2;
                cmd.command = 0x01; // Turn on cooling
                cmd.value = static_cast<uint16_t>((sensorData.temperature - 25.0f) * 100);
                cmd.timestamp = getCurrentTime();

                if (!controlQueue.trySend(cmd)) {
                    std::cout << "Control queue full! Command lost.\n";
                }
                else {
                    std::cout << "Control: Cooling command sent (value=" << cmd.value << ")\n";
                }
            }
        }

        // Check for critical temperature updates
        float currentTemp;
        uint32_t currentVersion = criticalTemperature.loadWithVersion(currentTemp);
        if (currentVersion != mLastTempVersion) {
            std::cout << "Control: Critical temperature updated to " << currentTemp << "°C\n";
            mLastTempVersion = currentVersion;
        }
    }

private:
    uint32_t mCounter = 0;
    uint32_t mLastTempVersion;

    uint32_t getCurrentTime() {
        return mCounter++ * 250; // ms
    }
};

/**
 * @brief Actuator task (500ms period)
 * Executes control commands received from control task
 */
class ActuatorTask : public ucosm::IPeriodicTask {
public:
    ActuatorTask() : ucosm::IPeriodicTask(500) {}

    void run() override {
        // Process all pending control commands
        ControlCommand cmd;
        while (controlQueue.tryReceive(cmd)) {
            std::cout << "Actuator: Executing command " << static_cast<int>(cmd.command)
                << " on device " << static_cast<int>(cmd.deviceId)
                << " with value " << cmd.value
                << " (latency=" << (getCurrentTime() - cmd.timestamp) << "ms)\n";

            // Simulate actuator action
            executeCommand(cmd);
        }

        // Check for calibration events
        if (systemEvents.testAny(EVENT_CALIBRATION)) {
            std::cout << "Actuator: Performing calibration...\n";
            systemEvents.clearFlags(EVENT_CALIBRATION);
        }
    }

private:
    uint32_t mCounter = 0;

    void executeCommand(const ControlCommand& cmd) {
        // Simulate command execution
        switch (cmd.command) {
            case 0x01:
                std::cout << "  -> Cooling system activated at " << cmd.value << "%\n";
                break;
            case 0xFF:
                std::cout << "  -> EMERGENCY SHUTDOWN ACTIVATED!\n";
                break;
            default:
                std::cout << "  -> Unknown command\n";
                break;
        }
    }

    uint32_t getCurrentTime() {
        return mCounter++ * 500; // ms
    }
};

/**
 * @brief Example of how to use RT communication in practice
 */
void demonstrateRTCommunication() {
    std::cout << "=== RT Inter-Task Communication Demo ===\n\n";

    // Create tasks
    SensorTask sensorTask;
    ControlTask controlTask;
    ActuatorTask actuatorTask;

    // In a real application, you would:
    // 1. Create RT scheduler and timer
    // 2. Add tasks to scheduler
    // 3. Let scheduler run the tasks

    // For demonstration, manually run a few cycles
    std::cout << "Running sensor task cycles...\n";
    for (int i = 0; i < 3; ++i) {
        sensorTask.run();
    }

    std::cout << "\nRunning control task...\n";
    controlTask.run();

    std::cout << "\nRunning actuator task...\n";
    actuatorTask.run();

    std::cout << "\nMessage queue states:\n";
    std::cout << "Sensor queue: " << sensorQueue.size() << " messages\n";
    std::cout << "Control queue: " << controlQueue.size() << " messages\n";

    std::cout << "\nShared variables:\n";
    std::cout << "Critical temperature: " << criticalTemperature.load() << "°C\n";
    std::cout << "System events: 0x" << std::hex << systemEvents.getFlags() << std::dec << "\n";

    std::cout << "\n=== Demo Complete ===\n";
}
