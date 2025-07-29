# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses CMake with C++17 standard. The build system is configured to:

- Build the main library as a header-only interface library (`ucosm_impl`)
- Include submodule `ulink` for automatic lifetime management
- Run comprehensive tests across multiple platforms (Linux/Windows with GCC/Clang/MSVC)

### Build Commands

```bash
# Configure CMake (creates build directory)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++

# Build the project
cmake --build build --config Release

# Run tests
cd build && ctest --build-config Release
```

### Testing

The project uses a custom test suite with doctest framework:
- Test executable: `ucosm_tests` 
- All tests are in `/tests/` directory
- Single test command: `cd build && ./ucosm_tests`

## Architecture Overview

µCosm is a lightweight C++17 scheduler framework for microcontrollers with zero heap allocation. The architecture is modular with these main components:

### Core Scheduling System

- **IScheduler** (`include/ucosm/core/ischeduler.hpp`): Template-based scheduler foundation using intrusive linked lists from `ulink` library
- **ITask** (`include/ucosm/core/itask.hpp`): Base task interface with rank-based ordering
- Tasks automatically remove themselves from schedulers when destroyed (RAII)

### Scheduler Types

1. **Periodic Scheduler** (`include/ucosm/periodic/`): Time-based cooperative scheduling
2. **CFS Scheduler** (`include/ucosm/cfs/`): Priority-based fair sharing with automatic period computation  
3. **RT Scheduler** (`include/ucosm/rt/`): Real-time scheduler using hardware timers/interrupts

### Task Types

1. **Periodic Tasks** (`iperiodic_task.hpp`): Execute at defined intervals
2. **CFS Tasks** (`icfs_task.hpp`): Priority-based cooperative tasks
3. **Resumable Tasks** (`iresumable_task.hpp`): Coroutine-like stateful execution with macro system
4. **Callable Tasks** (`callable_task.hpp`): Type-erased wrappers for lambdas/function pointers

### Key Design Patterns

- **Header-only library**: All implementation in headers except `rt_scheduler.cpp`
- **Template-based polymorphism**: Schedulers are templates parameterized by task type
- **Intrusive containers**: Tasks contain their own linking information (via `ulink`)
- **Hierarchical scheduling**: Schedulers can be nested as tasks in other schedulers
- **Lock-free communication**: RT message queues for inter-task communication

### Resumable Task Macros

The resumable task system uses a macro-based coroutine implementation:
- `UCOSM_START`: Begin resumable task (required first macro)
- `UCOSM_YIELD`: Yield execution, resume on next execution
- `UCOSM_SLEEP_FOR(ticks)`: Wait for specified scheduler ticks
- `UCOSM_SLEEP_UNTIL(condition, period)`: Wait until condition becomes true
- `UCOSM_RESTART`: Restart task from beginning
- `UCOSM_END`: End task and remove from scheduler

### Memory Management

- Uses `ulink` submodule for intrusive linked lists and automatic task lifetime management
- Zero heap allocation design suitable for embedded systems
- Tasks automatically unlink from schedulers on destruction

## Development Notes

- Current branch `ithread` appears to be developing new ithread functionality
- Modified files: `include/ucosm/resumable/ithread.h`, `tests/thread_test.cpp`
- The framework supports platform-independent development with the same API on desktop and microcontrollers
- RT scheduler requires platform-specific timer implementation via `ITimer` interface