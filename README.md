![build status](https://github.com/ThomasAUB/ucosm/actions/workflows/build.yml/badge.svg)
[![License](https://img.shields.io/github/license/ThomasAUB/ucosm)](LICENSE)
[![Version](https://img.shields.io/github/v/tag/ThomasAUB/ucosm?label=version&sort=semver)](https://github.com/ThomasAUB/ucosm/tags)

# µCosm

A lightweight, header-only C++17 scheduler framework for microcontrollers.

- **No heap allocation** and **no task limit**: tasks are intrusively linked.
- **Two schedulers**: cooperative periodic scheduling, and tasklets that defer interrupt-triggered work.
- **Resumable tasks**: coroutine-like tasks built on a few macros.
- **Callable tasks**: lambdas, function pointers and member functions as tasks.
- **Lock-free, ISR-safe** queues and shared variables.
- **Nestable** schedulers, and tasks that unlink themselves when destroyed.
- **Platform independent**: runs on desktop and on microcontrollers.

| Directory | Content |
|-----------|---------|
| [`core/`](include/ucosm/core) | `IScheduler`, `ITask` base classes and `CallableTask` |
| [`periodic/`](include/ucosm/periodic) | `PeriodicScheduler`, `IPeriodicTask` |
| [`tasklet/`](include/ucosm/tasklet) | `TaskletScheduler`, `ITasklet`, `TaskletQueue` |
| [`resumable/`](include/ucosm/resumable) | `IResumableTask`, `IResumableTasklet` and the coroutine macros |
| [`sync/`](include/ucosm/sync) | `MessageQueue`, `SharedVariable` (scheduler independent) |

## Getting Started

µCosm depends on the [ulink](https://github.com/ThomasAUB/ulink) and [uatom](https://github.com/ThomasAUB/uatom) submodules:

```sh
git clone --recursive https://github.com/ThomasAUB/ucosm.git
```

With CMake, link the `ucosm_impl` interface target (an existing `uatom` target is reused):

```cmake
add_subdirectory(ucosm)
target_link_libraries(my_app PRIVATE ucosm_impl)
```

Without CMake, add `include/`, `ulink/include/` and uatom's headers to the include path.

Run the tests with `cmake -B build && cmake --build build && ctest --test-dir build`.

## Periodic Scheduler

Tasks run cooperatively at fixed intervals, timed by a tick function you provide.

```cpp
#include "ucosm/periodic/periodic_scheduler.hpp"

struct Task final : ucosm::IPeriodicTask {
    void run() override {
        if (++mCounter == 5)  { this->setPeriod(10); } // change the period
        if (mCounter == 10)   { this->removeTask(); }  // leave the scheduler
    }
    int mCounter = 0;
};

int main() {
    ucosm::PeriodicScheduler sched(getTick_ms);

    Task t1, t2;
    t1.setPeriod(50);
    t2.setPeriod(1000);
    sched.addTask(t1);
    sched.addTask(t2);

    while (!sched.empty()) {
        sched.run();
    }
}
```

A new task runs right away, and its period applies afterwards. `sched.setDelay(task, delay)` shifts a task's next execution. An optional idle function, passed to the constructor or `setIdleTask()`, runs when no task is due.

## Tasklet Scheduler

Tasklets move long or blocking work out of interrupts: an ISR posts the work, and it runs later from a low-priority context such as `PendSV` on Cortex-M.

```cpp
#include "ucosm/tasklet/tasklet_scheduler.hpp"

constexpr ucosm::event_id_t button_event = 0;

ucosm::TaskletBackend backend {
    get_tick,                        // required: reads the platform clock
    request_low_priority_execution,  // e.g. sets the PendSV pending bit
    install_low_priority_handler,    // registers the handler PendSV must call
    suspend_low_priority_execution,  // e.g. raises BASEPRI
    resume_low_priority_execution    // restores it
};

ucosm::TaskletScheduler<1> sched(backend);  // template arg: number of event IDs

struct BlinkTask : ucosm::ITasklet {
    void run() override { toggleLed(); }
} blink;

struct ButtonTask : ucosm::ITasklet {
    void run() override { handleButton(); }
} button;

extern "C" void EXTI0_IRQHandler() { sched.signalEvent(button_event); }
extern "C" void SysTick_Handler()  { sched.poll(); }

int main() {
    blink.setPeriod(500);              // timer-driven
    button.waitForEvent(button_event); // event-driven
    sched.addTask(blink);
    sched.addTask(button);
    // ...
}
```

See [`tests/arm_tasklet_executor.hpp`](tests/arm_tasklet_executor.hpp) for a complete Cortex-M backend and [`tests/desktop_tasklet_executor.hpp`](tests/desktop_tasklet_executor.hpp) for a desktop model.

### Backend

| Hook | Required | Role |
|------|----------|------|
| `getTick` | yes | Reads the clock, in the unit of task periods |
| `requestTaskletExecution` | no | Pends the low-priority handler |
| `installHandler` | no | Registers the handler the pended interrupt must call |
| `suspendExecution` / `resumeExecution` | no | Masks the handler around critical sections |
| `scheduleNextWakeup` | no | Receives each new next deadline, for one-shot timer platforms |

Null optional hooks are replaced by no-ops. The clock alone never wakes a task, so the platform must either call `poll()` from a periodic tick interrupt, or implement `scheduleNextWakeup` to arm a one-shot timer (and never call `poll()`).

### Task configuration

A tasklet runs on a period (`setPeriod`) or waits for an event (`waitForEvent`); `addTask` refuses one with neither.

- The configuration persists across runs unless `run()` changes it. Events are coalesced.
- Periods are counted from the previous deadline, so dispatch latency doesn't accumulate. A task finishing more than a period late restarts from the current time, and missed deadlines are dropped. A period of 0 means the next tick.
- `addTask(task, delay)` and `setDelay(task, delay)` delay only the next execution.

### Stopping a task

- From its own `run()`: `removeTask()`, or `dispose()` to also clear its configuration.
- From elsewhere: `scheduler.removeTask(task)`, which takes the execution lock and refreshes the next deadline.

> [!WARNING]
> A tasklet's destructor unlinks it without the execution lock. Always call `scheduler.removeTask(task)` before destroying a scheduled tasklet.

### Priority levels

Tasklets of one scheduler never preempt each other. For preemptive levels, use one `TaskletScheduler` per level, each pending a different interrupt (e.g. PendSV for the lowest, other vectors via `NVIC_SetPendingIRQ` for higher ones).

## Resumable Tasks

Coroutine-like tasks that yield and resume where they left off, useful for state machines and protocols, with no heap allocation. Derive from `IResumableTask` (periodic) or `IResumableTasklet` (tasklet).

| Macro | Description |
|-------|-------------|
| `UCOSM_START` | Begins the resumable body (must come first) |
| `UCOSM_YIELD` | Resumes on the next execution (one tick on a tasklet) |
| `UCOSM_SLEEP_FOR(tick)` | Waits for `tick` scheduler ticks |
| `UCOSM_SLEEP_UNTIL(condition, check_period)` | Checks `condition` every `check_period` ticks until true |
| `UCOSM_WAIT_EVENT(id)` | Tasklets only: waits for `signalEvent(id)` |
| `UCOSM_RESTART` | Restarts from the beginning |
| `UCOSM_END` | Ends the task and removes it from its scheduler |

```cpp
#include "ucosm/resumable/iresumable_task.hpp"

struct ConnectTask : ucosm::IResumableTask {
    void run() override {
        UCOSM_START;

        startConnection();
        UCOSM_SLEEP_UNTIL(isConnected(), 100);  // check every 100 ticks

        sendData();
        UCOSM_SLEEP_FOR(1000);

        if (!responseReceived()) {
            UCOSM_RESTART;
        }

        UCOSM_END;
    }
};

struct RxTask : ucosm::IResumableTasklet {
    void run() override {
        UCOSM_START;
        UCOSM_WAIT_EVENT(rx_event);
        processFrame();
        UCOSM_RESTART;
    }
};
```

## Callable Tasks

`CallableTask<task_t>` wraps a lambda, function pointer or member function into any task type. Small callables are stored inline.

```cpp
#include "ucosm/core/callable_task.hpp"

ucosm::CallableTask<ucosm::IPeriodicTask> lambdaTask([&] { ++counter; });
ucosm::CallableTask<ucosm::IPeriodicTask> memberTask(&MyClass::doWork, myObject);

lambdaTask.setPeriod(100);
sched.addTask(lambdaTask);
```

## Inter-Context Communication

The `sync` primitives pass data between contexts (ISR to task, task to task) without locks or allocation.

| Type | Description |
|------|-------------|
| `MessageQueue<T, Size>` | Lock-free SPSC queue (`Size` a power of 2), with bulk `trySend`/`tryReceive` overloads |
| `SharedVariable<T>` | Single-writer lock-free value; `hasChanged(lastVersion)` detects updates |
| `TaskletQueue<T, Size, event_count>` | `MessageQueue` that signals a tasklet event on each send |

```cpp
#include "ucosm/tasklet/tasklet_queue.hpp"

constexpr ucosm::event_id_t uart_rx_event = 0;

ucosm::TaskletScheduler<1> sched(backend);
ucosm::TaskletQueue<uint8_t, 64, 1> rxQueue(sched, uart_rx_event);

struct RxTask : ucosm::ITasklet {
    void run() override {
        uint8_t byte;
        while (rxQueue.tryReceive(byte)) {  // events coalesce: drain the queue
            parse(byte);
        }
    }
} rxTask;

extern "C" void UART_IRQHandler() { rxQueue.trySend(UART->DR); }

int main() {
    rxTask.waitForEvent(uart_rx_event);
    sched.addTask(rxTask);
    // ...
}
```

Sends and receives use no read-modify-write operations and usually a single memory barrier. When all sharing contexts run on one core, define `UCOSM_SINGLE_CORE=1` to drop the barriers too. Leave it undefined on multi-core parts (RP2040, dual-core STM32H7).

## Hierarchical Scheduling

A scheduler is itself a task, so schedulers can be nested into trees. A scheduler can also run from another's idle function, getting only the time the higher level leaves free:

```cpp
ucosm::PeriodicScheduler lowSched(getTick_ms);
ucosm::PeriodicScheduler highSched(getTick_ms, +[] { lowSched.run(); });

int main() {
    while (true) {
        highSched.run();
    }
}
```

## Task Lifetime

A task removes itself from its scheduler when destroyed (tasklets: see the [warning above](#stopping-a-task)):

```cpp
void foo() {
    Task tempTask;
    sched.addTask(tempTask);
} // tempTask is unlinked here
```

## License

[MIT](LICENSE)
