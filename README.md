![build status](https://github.com/ThomasAUB/ucosm/actions/workflows/build.yml/badge.svg)
[![License](https://img.shields.io/github/license/ThomasAUB/ucosm)](LICENSE)

# uCosm

Lightweight C++17 scheduler library for microcontrollers supporting cooperative and real-time scheduling.

**Key Features:**
- No heap allocation
- No task number limitations
- Platform independent
- Hierarchical scheduling trees
- Multiple scheduling policies: Cooperative, Fair, and Real-time
- Resumable tasks with coroutine-like behavior
- Callable task wrappers for lambdas and functions
- Customizable scheduling algorithms

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

This library provides a modular scheduling framework with three main implementations:

| Scheduler | Type | Execution | Use Case |
|-----------|------|-----------|----------|
| **Periodic** | Cooperative | Time-based intervals | Regular maintenance tasks |
| **CFS** | Cooperative | Priority-based fair sharing | CPU-intensive workloads |
| **RT** | Real-time | Hardware timer interrupts | Deterministic real-time systems |

- **Core**: Basic cooperative scheduler foundation
- **Periodic**: Time-based cooperative task scheduling  
- **CFS**: Completely Fair Scheduler with priority-based execution
- **RT**: Real-time scheduler with hardware timer integration
- **Resumable Tasks**: Coroutine-like tasks that can yield execution and resume later

# Examples

## Periodic tasks

Time-based cooperative scheduling where tasks execute at defined intervals.

```cpp
#include <iostream>
#include "periodic/iperiodic_task.hpp"

struct Task final : ucosm::IPeriodicTask {
    void run() override {
        std::cout << "run " << this->getPeriod() << std::endl;
        if(mCounter++ == 10) {
            this->remove();
        }
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

    t1.setPeriod(50);    // execute every 50 milliseconds
    t2.setPeriod(1000);  // execute every seconds

    sched.addTask(t1);
    sched.addTask(t2);

    while(!sched.empty()) {
        sched.run();
    }

    return 0;
}
```

## CFS (Completely Fair Scheduler)

Priority-based cooperative scheduling with automatic time-slicing for fair execution.

```cpp
#include <iostream>
#include "cfs/icfs_task.hpp"

struct Task final : ucosm::ICFSTask {
    void run() override {
        std::cout << "run " << (uint16_t)this->getPriority() << std::endl;
        if(mCounter++ == 10) {
            this->remove();
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

## Real-Time (RT) Scheduler

Interrupt timer-based scheduling for real-time applications.

The RT scheduler provides deterministic task execution using platform-specific timers. On microcontrollers, it uses dedicated hardware timers with interrupt priorities. On desktop platforms, the same interface can be implemented using high-resolution threads for development and testing purposes.

Timing accuracy depends on the collective workload of all tasks within a scheduler. For maximum determinism, multiple schedulers can be created with timers of different interrupt priorities.

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
    void disable() override { /* Disable timer */ }
    void enable() override { /* Enable timer */ }
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

## Resumable Tasks

Resumable tasks provide coroutine-like functionality, allowing tasks to yield execution and resume later at the same point. This is particularly useful for implementing complex state machines, protocols, or multi-step operations without blocking other tasks.

**Key Features:**
- Zero heap allocation
- Minimal memory overhead (8 bytes per task)
- Cooperative multitasking with explicit yield points
- Time-based delays and waiting
- Safe macro system for state management
- Built on Duff's Device pattern for efficient state machines

```cpp
#include <iostream>
#include "ucosm/periodic/ilong_task.hpp"

struct SequenceTask : ucosm::IResumableTask {
    SequenceTask() : ucosm::IResumableTask(10) {} // 10ms base period
    
    void run() override {
        UCOSM_START;
            std::cout << "Step 1: Initialize" << std::endl;
            
        UCOSM_YIELD;  // Yield to other tasks, resume next time
            std::cout << "Step 2: Process" << std::endl;
            
        UCOSM_WAIT(1000)  // Wait 1 second, then continue
            std::cout << "Step 3: Complete" << std::endl;
            
        UCOSM_END;  // Task completes and removes itself
    }
};
```

### Advanced Resumable Task Patterns

**State Machine Example:**
```cpp
struct StateMachineTask : ucosm::IResumableTask {
    enum State { IDLE, CONNECTING, SENDING, WAITING, DONE };
    State currentState = IDLE;
    int attempts = 0;
    
    void run() override {

        UCOSM_START;

        currentState = CONNECTING;
        std::cout << "Connecting..." << std::endl;
            
        UCOSM_WAIT(500);  // Connection delay

        if (connectionSuccessful()) {
            currentState = SENDING;
            std::cout << "Sending data..." << std::endl;
        } else {
            std::cout << "Connection failed, retrying..." << std::endl;
            UCOSM_RESTART;  // Restart from beginning
        }

        UCOSM_WAIT(200);  // Send delay

        currentState = WAITING;
        std::cout << "Waiting for response..." << std::endl;
        
        UCOSM_WAIT(1000);  // Response timeout

        if (responseReceived()) {
            std::cout << "Success!" << std::endl;
            currentState = DONE;
        } else if (++attempts < 3) {
            std::cout << "Timeout, retrying..." << std::endl;
            currentState = SENDING;
            UCOSM_RESTART;
        } else {
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
| `UCOSM_YIELD` | Yield execution, resume next time task runs |
| `UCOSM_WAIT(tick)` | Wait for specified scheduler ticks before continuing |
| `UCOSM_WAIT_UNTIL(condition, check_period)` | Wait here until the *condition* is true |
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

# Hierarchical Scheduling

Schedulers can be nested as tasks within other schedulers, creating flexible scheduling hierarchies.

**Example**: Periodic scheduler containing a CFS scheduler containing another periodic scheduler.

```cpp
ucosm::PeriodicScheduler<ucosm::ICFSTask> periodicScheduler(getTick_ms);

ucosm::CFSScheduler<ucosm::IPeriodicTask> cfsScheduler(getTick_us);

ucosm::PeriodicScheduler periodicScheduler2(getTick_ms);

cfsScheduler.addTask(periodicScheduler);

periodicScheduler2.addTask(cfsScheduler);
```

# Memory Safety

Task storage uses [ulink](https://github.com/ThomasAUB/ulink) for automatic lifetime management. Tasks automatically remove themselves from schedulers when destroyed.

```cpp
void foo() {
    Task tempTask;
    sched.addTask(tempTask);
}// tempTask removes itself from the scheduler at the end of the scope
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
            if (counter >= 5) {
               // Task removes itself when done
            }
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
