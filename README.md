![build status](https://github.com/ThomasAUB/ucosm/actions/workflows/build.yml/badge.svg)
[![License](https://img.shields.io/github/license/ThomasAUB/ucosm)](LICENSE)

# µCosm

A lightweight C++17 scheduler framework for microcontrollers that supports cooperative and real-time scheduling.

**Key Features:**
- **Zero heap allocation** - All operations use static memory
- **Unlimited task count** - No arbitrary limits on task numbers  
- **Platform independent** - Unified API for desktop and microcontrollers
- **Hierarchical scheduling** - Nest schedulers within schedulers
- **Multiple policies** - Periodic, CFS, and RT scheduling algorithms
- **Resumable tasks** - Coroutine-like behavior with macro system
- **Callable wrappers** - Lambda and function pointer support
- **Real-time communication** - Lock-free inter-task messaging
- **Memory safety** - Automatic task lifetime management
- **High performance** - Optimized for embedded systems


This library provides a modular scheduling framework with three main implementations:

| Scheduler | Type | Execution | Use Case |
|-----------|------|-----------|----------|
| **Periodic** | Cooperative | Time-based intervals | Regular maintenance tasks |
| **CFS** | Cooperative | Priority-based fair sharing | CPU-intensive workloads |
| **RT** | Real-time | Hardware timer interrupts | Deterministic real-time systems |

**Additional Components:**
- **Core**: Basic cooperative scheduler foundation with intrusive containers
- **Resumable Tasks**: Macro-based coroutine system for stateful operations  
- **Callable Tasks**: Type-erased wrappers for lambdas and function pointers
- **RT Communication**: Lock-free message queues for inter-task communication

# Examples

## Periodic Tasks

Time-based cooperative scheduling where tasks execute at defined intervals.

```cpp
#include <iostream>
#include "periodic/iperiodic_task.hpp"

struct Task final : ucosm::IPeriodicTask {

    void run() override {

        std::cout << "run " << this->getPeriod() << std::endl;

        if(mCounter == 5) {
            // Dynamically change the execution period
            this->setPeriod(10);
        }

        if(mCounter == 10) {
            // Remove the task from its scheduler
            this->removeTask();
        }

        mCounter++;
    }
    int mCounter = 0;
};
```

```cpp
#include <chrono>
#include "periodic/periodic_scheduler.hpp"

static ucosm::IPeriodicTask::tick_t getTick_ms() {
    return static_cast<ucosm::IPeriodicTask::tick_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

int main() {

    ucosm::PeriodicScheduler sched(getTick_ms);

    Task t1;
    Task t2;

    t1.setPeriod(50);    // Execute every 50 milliseconds
    t2.setPeriod(1000);  // Execute every second

    sched.addTask(t1);
    sched.addTask(t2);

    while(!sched.empty()) {
        sched.run();
    }

    return 0;
}
```

## CFS Tasks

Priority-based cooperative scheduling that automatically computes task periods based on execution time and priority. This ensures fair CPU usage among tasks of the same priority by executing longer-running tasks less frequently.

```cpp
#include <iostream>
#include "cfs/icfs_task.hpp"

struct Task final : ucosm::ICFSTask {
    void run() override {
        std::cout << "run " << (uint16_t)this->getPriority() << std::endl;
        if(mCounter++ == 10) {
            this->removeTask();
        }
    }
    int mCounter = 0;
};
```

```cpp
#include <chrono>
#include "cfs/cfs_scheduler.hpp"

int main() {

    ucosm::CFSScheduler sched(getTick_us);

    Task t1;
    Task t2;

    t1.setPriority(2);
    t2.setPriority(4);

    sched.addTask(t1);
    sched.addTask(t2);

    while(!sched.empty()) {
        sched.run();
    }

    return 0;
}
```


## Resumable Tasks

Resumable tasks provide coroutine-like functionality, allowing tasks to yield execution and resume later at the same point. This is particularly useful for implementing complex state machines, communication protocols, or multi-step operations without blocking other tasks.

**Key Features:**
- Zero heap allocation
- Minimal memory overhead
- Cooperative multitasking with explicit yield points
- Time-based delays and waiting
- Safe macro system for state management

```cpp
#include <iostream>
#include "ucosm/periodic/ilong_task.hpp"

struct SequenceTask : ucosm::IResumableTask {
    
    void run() override {

        UCOSM_START;

        std::cout << "Step 1: Initialize" << std::endl;

        UCOSM_YIELD;  // Yield to other tasks, resume on next execution

        std::cout << "Step 2: Process" << std::endl;

        UCOSM_SLEEP_FOR(1000)  // Wait 1 second, then continue

        std::cout << "Step 3: Complete" << std::endl;

        UCOSM_END;  // Task completes and removes itself
    }
};
```

### Advanced Resumable Task Patterns

**State Machine Example:**
```cpp
struct StateMachineTask : ucosm::IResumableTask {

    int mAttempts = 0;
    int mRetries = 0;
    
    void run() override {

        UCOSM_START;

        std::cout << "Connecting..." << std::endl;

        UCOSM_SLEEP_FOR(500);  // Connection delay

        if (connectionSuccessful()) {
            std::cout << "Sending data..." << std::endl;
        }
        else {
            std::cout << "Connection failed, retrying..." << std::endl;
            UCOSM_RESTART;  // Restart from beginning
        }

        UCOSM_SLEEP_FOR(200);  // Send delay

        std::cout << "Waiting for response..." << std::endl;

        mRetries = 0;
        mAttempts = 0;

        // Timeout after 300ms
        UCOSM_SLEEP_UNTIL(responseReceived() || mRetries++ == 3, 100);

        if (responseReceived()) {
            std::cout << "Success!" << std::endl;
        }
        else if (++mAttempts < 3) {
            std::cout << "Timeout, retrying..." << std::endl;
            UCOSM_RESTART;
        }
        else {
            std::cout << "Max retries reached" << std::endl;
        }

        UCOSM_END;
    }
};
```

### Resumable Task Macros

| Macro | Description |
|-------|-------------|
| `UCOSM_START` | Begin the resumable task (required first macro) |
| `UCOSM_YIELD` | Yield execution, resume on next task execution |
| `UCOSM_SLEEP_FOR(tick)` | Wait for specified scheduler ticks before continuing |
| `UCOSM_SLEEP_UNTIL(condition, check_period)` | Wait until the condition becomes true |
| `UCOSM_RESTART` | Restart task from the beginning |
| `UCOSM_END` | End task and remove from scheduler |


### Using Resumable Tasks with Schedulers

Resumable tasks inherit from `IPeriodicTask`, so they can be used with any scheduler that accepts periodic tasks:

```cpp
#include "ucosm/periodic/periodic_scheduler.hpp"

int main() {
    ucosm::PeriodicScheduler sched(getTick_ms);
    
    SequenceTask task1;
    StateMachineTask task2;
    
    sched.addTask(task1);
    sched.addTask(task2);
    
    while(!sched.empty()) {
        sched.run();
    }
    
    return 0;
}
```

## Real-Time (RT) Scheduler

The RT scheduler provides deterministic task execution using platform-specific timers. On microcontrollers, it uses dedicated hardware timers with configurable interrupt priorities. On desktop platforms, the same interface can be implemented using high-resolution threads for development and testing.

Tasks within the same scheduler are executed cooperatively, so timing accuracy depends on the collective workload of all tasks within the scheduler. For maximum determinism, multiple schedulers can be created with timers using different interrupt priorities.

```cpp
#include <iostream>
#include "ucosm/periodic/iperiodic_task.hpp"

struct RTTask final : ucosm::IPeriodicTask {
    RTTask(uint32_t period_ms) : ucosm::IPeriodicTask(period_ms) {}
    
    void run() override {
        std::cout << "RT task executed at " << getPeriod() << "ms period\n";
        if(mCounter++ == 10) {
            this->removeTask();
        }
    }
    int mCounter = 0;
};
```

```cpp
#include "ucosm/rt/rt_scheduler.hpp"

// Hardware timer or thread implementation (platform-specific)
class Timer : public ucosm::RTScheduler::ITimer {
    // Implement virtual methods for your platform
    void start() override { /* Start timer */ }
    void stop() override { /* Stop timer */ }
    bool isRunning() const override { /* Check timer status */ }
    void setDuration(uint32_t duration) override { /* Set timer period */ }
    void disable() override { /* Disable timer interrupt */ }
    void enable() override { /* Enable timer interrupt */ }
};

int main() {
    Timer timer;
    ucosm::RTScheduler scheduler;
    scheduler.setTimer(timer);

    RTTask task1(100);  // Execute every 100ms
    RTTask task2(500);  // Execute every 500ms

    scheduler.addTask(task1);
    scheduler.addTask(task2);

    // Tasks execute automatically via timer interrupts
    while(!scheduler.empty()) {
        // Main loop can handle other work
    }

    return 0;
}
```

## Real-Time Communication

For inter-task communication, µCosm provides lock-free message queues optimized for real-time systems:

```cpp
#include "ucosm/rt/rt_inter_task.hpp"

// Define message structure  
struct SensorData {
    uint32_t timestamp;
    float temperature;
    float humidity;
};

// Create lock-free queue (size must be power of 2)
ucosm::RTMessageQueue<SensorData, 16> sensorQueue;

// Producer task (high priority)
struct SensorTask : ucosm::IPeriodicTask {
    void run() override {
        SensorData data = readSensors();
        if (!sensorQueue.trySend(data)) {
            // Queue full - handle overflow condition
        }
    }
};

// Consumer task (lower priority)  
struct ProcessorTask : ucosm::IPeriodicTask {
    void run() override {
        SensorData data;
        while (sensorQueue.tryReceive(data)) {
            processSensorData(data);
        }
    }
};
```

## Callable Tasks

For simple tasks that don't require full class definitions, `CallableTask` provides a convenient wrapper that can store lambdas, function pointers, and member functions.

**Key Features:**
- Small buffer optimization (no heap allocation for small callables)
- Support for lambdas, function pointers, and member functions  
- Safe empty callable handling

```cpp
#include "ucosm/core/callable_task.hpp"

int main() {
    ucosm::PeriodicScheduler sched(getTick_ms);
    
    int counter = 0;
    
    // Lambda task
    ucosm::CallableTask<ucosm::IPeriodicTask> lambdaTask(
        [&counter]() {
            std::cout << "Counter: " << ++counter << std::endl;
        }
    );
    
    // Member function task
    struct MyClass {
        void doWork() { std::cout << "Member function called" << std::endl; }
    } myObject;
    
    ucosm::CallableTask<ucosm::IPeriodicTask> memberTask(&MyClass::doWork, myObject);
    
    lambdaTask.setPeriod(100);
    memberTask.setPeriod(200);
    
    sched.addTask(lambdaTask);
    sched.addTask(memberTask);
    
    while (!sched.empty()) {
        sched.run();
    }
    
    return 0;
}
```

# Hierarchical Scheduling

Schedulers can be nested as tasks within other schedulers, enabling sophisticated scheduling topologies for complex systems.

```mermaid
flowchart LR

scheduler(Scheduler)

task1(Task)
task2(Task)
task3(Task)
schedTask(Scheduler)
schedTask
subTask1(Task)
subTask2(Task)

scheduler --> task1
scheduler --> task2
scheduler --> task3
scheduler --> schedTask

schedTask --> subTask1
schedTask --> subTask2
```

Schedulers can also be executed from the idle function of higher-priority schedulers.

```cpp

ucosm::PeriodicScheduler& lowPrioScheduler() {
    static ucosm::PeriodicScheduler sLowSched(getTick_ms);
    return sLowSched;
}

ucosm::PeriodicScheduler& mediumPrioScheduler() {
    static ucosm::PeriodicScheduler sMediumSched(
        getTick_ms,
        +[]() { // idle function
            lowPrioScheduler().run();
        }
    );
    return sMediumSched;
}

ucosm::PeriodicScheduler& highPrioScheduler() {
    static ucosm::PeriodicScheduler sHighSched(
        getTick_ms,
        +[]() { // idle function
            mediumPrioScheduler().run();
        }
    );
    return sHighSched;
}

int main() {

    while(true) {
        highPrioScheduler().run();
    }

    return 0;
}

```

# Memory Safety

Task storage uses [ulink](https://github.com/ThomasAUB/ulink) for automatic lifetime management. Tasks automatically remove themselves from schedulers when destroyed.

```cpp
void foo() {
    Task tempTask;
    sched.addTask(tempTask);
} // tempTask removes itself from the scheduler at the end of the scope
```

