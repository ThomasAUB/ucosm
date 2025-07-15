# RT Inter-Task Communication Guide

This document explains how to use the lock-free communication primitives in uCosm's RT scheduler for safe data exchange between tasks of different priorities.

## Overview

The RT communication system provides three main primitives:

1. **RTMessageQueue** - Lock-free SPSC queue for message passing
2. **RTSharedVariable** - Atomic shared variables with versioning
3. **RTEventFlags** - Bit-based event signaling system

All primitives are designed to work in hard real-time environments without dynamic memory allocation, locks, or blocking operations.

## RTMessageQueue

Lock-free single-producer-single-consumer message queue for passing structured data between tasks.

### Key Features
- **Wait-free operations**: No blocking, perfect for RT systems
- **Type-safe**: Template-based with compile-time type checking
- **Fixed capacity**: No dynamic allocation, size must be power of 2
- **Cache-friendly**: Aligned to prevent false sharing

### Basic Usage

```cpp
#include "ucosm/rt/rt_message_queue.hpp"

// Define message structure
struct SensorReading {
    uint32_t timestamp;
    float temperature;
    uint16_t sensorId;
};

// Create queue (size must be power of 2)
ucosm::RTMessageQueue<SensorReading, 16> sensorQueue;

// Producer task (higher priority)
void sensorTask() {
    SensorReading reading;
    reading.timestamp = getCurrentTime();
    reading.temperature = readTemperature();
    reading.sensorId = 1;
    
    if (!sensorQueue.trySend(reading)) {
        // Queue full - handle gracefully
        // Never blocks in RT system
    }
}

// Consumer task (lower priority)
void processingTask() {
    SensorReading reading;
    while (sensorQueue.tryReceive(reading)) {
        // Process the reading
        processTemperature(reading.temperature);
    }
}
```

### Best Practices
- Use power-of-2 sizes for optimal performance
- Keep message types small and trivially copyable
- Producer should handle queue-full conditions gracefully
- Consumer should drain all available messages

## RTSharedVariable

Atomic shared variable with version tracking for detecting updates.

### Key Features
- **Lock-free access**: Atomic operations only
- **Version tracking**: Detect when value changes
- **Compare-and-swap**: Atomic conditional updates
- **Cache-efficient**: Single cache line per variable

### Basic Usage

```cpp
// Create shared variable with initial value
ucosm::RTSharedVariable<float> targetSpeed(0.0f);

// Writer task
void controlTask() {
    float newSpeed = calculateTargetSpeed();
    targetSpeed.store(newSpeed);
}

// Reader task
void actuatorTask() {
    float speed = targetSpeed.load();
    setMotorSpeed(speed);
}

// Reader with change detection
void monitoringTask() {
    static uint32_t lastVersion = 0;
    
    float speed;
    uint32_t currentVersion = targetSpeed.loadWithVersion(speed);
    
    if (currentVersion != lastVersion) {
        // Speed has changed
        logSpeedChange(speed);
        lastVersion = currentVersion;
    }
}

// Conditional update
void safetyTask() {
    float expected = targetSpeed.load();
    if (expected > MAX_SAFE_SPEED) {
        // Try to limit speed atomically
        targetSpeed.compareAndSwap(expected, MAX_SAFE_SPEED);
    }
}
```

### Version Tracking
- Version increments only when value actually changes
- Use `loadWithVersion()` for efficient change detection
- Version numbers are monotonically increasing

## RTEventFlags

Bit-based event signaling system for coordinating between tasks.

### Key Features
- **32-bit flags**: Up to 32 different events
- **Atomic operations**: Set, clear, test operations
- **Flexible waiting**: Wait for any or all flags
- **Non-blocking option**: Test without waiting

### Basic Usage

```cpp
// Global event flags
ucosm::RTEventFlags systemEvents;

// Define event bits
constexpr uint32_t EVENT_SENSOR_READY    = 0x01;
constexpr uint32_t EVENT_CONTROL_NEEDED  = 0x02;
constexpr uint32_t EVENT_EMERGENCY       = 0x04;
constexpr uint32_t EVENT_CALIBRATION     = 0x08;

// Signal events
void sensorTask() {
    // ... read sensor ...
    systemEvents.setFlags(EVENT_SENSOR_READY);
    
    if (emergencyDetected) {
        systemEvents.setFlags(EVENT_EMERGENCY);
    }
}

// Test for events
void controlTask() {
    if (systemEvents.testAny(EVENT_SENSOR_READY)) {
        processSensorData();
        systemEvents.clearFlags(EVENT_SENSOR_READY);
    }
    
    if (systemEvents.testAny(EVENT_EMERGENCY)) {
        handleEmergency();
        systemEvents.clearFlags(EVENT_EMERGENCY);
    }
}

// Wait for events (with timeout)
void maintenanceTask() {
    // Wait up to 1000ms for calibration request
    if (systemEvents.waitAny(EVENT_CALIBRATION, 1000)) {
        performCalibration();
        systemEvents.clearFlags(EVENT_CALIBRATION);
    }
}
```

### Event Patterns
- Use bit masks for related events
- Clear events after processing to avoid duplication
- Combine multiple events for complex conditions

## Integration with RT Scheduler

Example showing complete integration with the RT scheduler:

```cpp
#include "ucosm/rt/rt_scheduler.hpp"
#include "ucosm/rt/rt_message_queue.hpp"

// Communication channels
ucosm::RTMessageQueue<SensorData, 16> sensorQueue;
ucosm::RTSharedVariable<float> criticalValue(0.0f);
ucosm::RTEventFlags controlEvents;

class HighPriorityTask : public ucosm::IPeriodicTask {
public:
    HighPriorityTask() : IPeriodicTask(10) {} // 10ms period
    
    void run() override {
        // Critical sensor reading
        SensorData data = readCriticalSensor();
        
        // Send to processing task
        if (!sensorQueue.trySend(data)) {
            // Handle queue full
            handleDataLoss();
        }
        
        // Update shared value if critical
        if (data.value > CRITICAL_THRESHOLD) {
            criticalValue.store(data.value);
            controlEvents.setFlags(EVENT_CRITICAL);
        }
    }
};

class MediumPriorityTask : public ucosm::IPeriodicTask {
public:
    MediumPriorityTask() : IPeriodicTask(50) {} // 50ms period
    
    void run() override {
        // Process all available sensor data
        SensorData data;
        while (sensorQueue.tryReceive(data)) {
            processData(data);
        }
        
        // Check for critical events
        if (controlEvents.testAny(EVENT_CRITICAL)) {
            float critical = criticalValue.load();
            handleCriticalValue(critical);
            controlEvents.clearFlags(EVENT_CRITICAL);
        }
    }
};
```

## Memory and Performance Considerations

### Memory Layout
- Message queues: `sizeof(T) * Size + 2 * cache_line_size`
- Shared variables: `sizeof(T) + sizeof(uint32_t) + alignment`
- Event flags: `sizeof(uint32_t)` (4 bytes)

### Performance Characteristics
- **RTMessageQueue**: O(1) send/receive, wait-free
- **RTSharedVariable**: O(1) load/store, lock-free
- **RTEventFlags**: O(1) operations, lock-free

### Cache Considerations
- Each primitive aligned to prevent false sharing
- Keep hot data in same cache lines when possible
- Avoid sharing between cores when possible

## Error Handling

### Queue Full Conditions
```cpp
if (!queue.trySend(data)) {
    // Options:
    // 1. Drop oldest data (if safe)
    // 2. Count lost messages
    // 3. Signal data loss event
    // 4. Implement backpressure
}
```

### Compare-and-Swap Failures
```cpp
float expected = sharedVar.load();
if (!sharedVar.compareAndSwap(expected, newValue)) {
    // Another task modified the value
    // expected now contains current value
    // Decide whether to retry or abort
}
```

### Event Timeouts
```cpp
if (!events.waitAny(FLAGS, timeout)) {
    // Timeout occurred
    // Handle missing events gracefully
}
```

## Migration from Blocking Primitives

### From Mutex-Protected Data
```cpp
// Before (blocking)
std::mutex dataMutex;
float sharedData;

void writer() {
    std::lock_guard<std::mutex> lock(dataMutex);
    sharedData = newValue;
}

// After (lock-free)
ucosm::RTSharedVariable<float> sharedData(0.0f);

void writer() {
    sharedData.store(newValue);  // Always succeeds, no blocking
}
```

### From Condition Variables
```cpp
// Before (blocking)
std::condition_variable cv;
std::mutex cvMutex;
bool dataReady = false;

void waiter() {
    std::unique_lock<std::mutex> lock(cvMutex);
    cv.wait(lock, []{ return dataReady; });
}

// After (lock-free)
ucosm::RTEventFlags events;

void waiter() {
    if (events.waitAny(DATA_READY_FLAG, timeout)) {
        // Process data
    }
}
```

## Platform Considerations

### Microcontroller Usage
- No dynamic allocation - all memory statically allocated
- Atomic operations map directly to hardware instructions
- Event flags can integrate with hardware interrupts
- Memory barriers ensure correct ordering across cores

### Testing on PC
- Full std::atomic implementation provides strong guarantees
- Thread simulation allows validation of RT behavior
- Timing functions enable latency measurement
- Cross-platform code runs identically on target hardware

This communication system provides a solid foundation for building complex real-time applications while maintaining deterministic behavior and avoiding the pitfalls of traditional blocking synchronization primitives.
