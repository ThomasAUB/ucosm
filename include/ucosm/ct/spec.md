
# Compile-time task scheduler

## Purpose

Provide a small, portable, compile-time-driven task scheduling library written in Modern C++17. The library's core is intended to be zero-heap, have minimal runtime overhead, and allow tasks and resources to be declared and validated at compile time so that the resulting system is suitable for both small microcontrollers and desktop environments. The core is intentionally platform-neutral and does not implement platform-specific context switching; it provides the scheduling decisions and safe hooks for platform layers to implement preemption.

## Goals

- **Deterministic compile-time behavior:** Deduce task ordering, dependencies and conflicts at compile time whenever possible.
- **No runtime heap allocation:** Use only static storage and compile-time constructs; avoid dynamic allocation in the core.
- **Static task declaration:** Tasks, priorities and resource usage are declared at compile-time.
- **No user stack estimation:** The user does not need to estimate or declare per-task stacks for the scheduler core.
- **Portable:** Works on tiny microcontrollers and on desktop targets.
- **Preemptive-ready:** Provide scheduling semantics and hooks that make it straightforward to build a preemptive RTOS layer on top.

## Constraints

- **Language:** ISO C++17.
- **Memory:** No heap usage in the scheduler core. Only static and compile-time storage.
- **Platform:** Core library contains no platform-specific assembly for context switching.
- **Error handling:** Prefer compile-time checks and `noexcept` runtime operations for hot paths.

## Terminology

- **Task:** A compile-time-declared unit of work (handler callable). Tasks are identified by an ID or name and a priority.
- **Priority:** Integral value that determines order and preemption (lower value → higher priority).
- **Resource:** A statically-declared object or capability that tasks may access; resources are described with access attributes (read-only, exclusive, shared).
- **Scheduler:** The compile-time-built structure that performs validation and emits the runtime scheduling data structures.

## Design Principles

- **Compile-time declaration & validation:** Prefer `constexpr`/template-based APIs to declare tasks and resources, so dependency graphs, cycles and basic conflicts can be checked at compile time.
- **Minimal runtime:** The runtime provides only the operations required to run and dispatch tasks; heavy validation and ordering decisions are done at compile time.
- **Safe defaults:** Conservative defaults for locking and preemption; explicit opt-in for performance-costly features like priority inheritance.
- **No implicit stack management:** The library doesn't allocate per-task stacks or require the user to specify them. Resumable/coroutine-like task patterns are supported in examples but are optional.

## API Sketch (informal)

- `constexpr auto make_task<Name, Priority>(handler, resources...)` — declare a task at compile time.
- `constexpr auto make_resource<Name>(storage_ref, attributes)` — declare a static resource descriptor.
- `constexpr auto build_system(tasks..., resources...)` — performs compile-time analysis and constructs scheduler data structures.
- Runtime entry points: `system.init()`, `system.start()`, `system.yield()`, `system.schedule()`.
- ISR notify primitive: `system.interrupt_notify(task_id)` — safe for use from ISRs with documented atomicity requirements.

Example (pseudo):

- `constexpr auto sensor = make_resource<"sensor">(...);
- constexpr auto t_sensor = make_task<"sensor_read", 10>(sensor_poll, sensor);
- constexpr auto t_proc   = make_task<"process", 5>(process_data, buffer);
- constexpr auto sys = build_system(t_sensor, t_proc, sensor, buffer);`

## Compile-time Checks & Guarantees

- **Graph validation:** Detect cycles and report clear `static_assert` messages during compilation.
- **Resource conflict detection:** Flag incompatible access patterns (e.g., two tasks requiring exclusive access) at compile time where possible.
- **Priority inversion reporting:** Detect potential priority inversion situations and require explicit opt-in for inheritance.
- **Memory footprint estimation:** Provide compile-time computed estimates of static storage used by the scheduler.

## Scheduling Model

- **Primary model:** Priority-based scheduling where a higher-priority ready task preempts lower-priority tasks.
- **Cooperative/resumable mode:** For environments without preemption, provide cooperative APIs and examples built on resumable task patterns.
- **Preemption hooks:** The core supplies decision points and APIs; platform layers implement the actual context switch.

## Resource Model

- **Static resources:** Resources are declared at compile time with usage attributes (read, write, exclusive).
- **Locking primitives:** Provide lightweight compile-time-known lock descriptors and RAII wrappers for runtime use.
- **Priority inheritance:** Optional, explicit strategy to avoid overhead when not needed.

## Safety & Error Handling

- Hot-path APIs should be `noexcept` and avoid throwing exceptions.
- Use `static_assert` for configuration errors; provide clear diagnostics.
- Document UB (for example, concurrent non-reentrant handler usage without locking).

## Testing & Verification

- **Unit tests:** Desktop-hosted unit tests for graph analysis, compile-time checks and runtime behavior.
- **Static analysis:** Recommend CI jobs with sanitizers and static analyzers for the host builds.
- **MCU integration tests:** Example hardware integration or emulation tests for ISR and preemption glue.

## Non-Goals

- Implementing platform-specific context switching.
- Supporting runtime addition/removal of tasks in the core scheduler.
- Using heap-based resource pools in the core.

## Examples & Repo Suggestions

- `examples/minimal_scheduler.cpp` — minimal desktop example showing compile-time task declaration and runtime dispatch.
- `examples/mcu_integration.md` — guidance for wiring the scheduler decisions into a Cortex-M preemptive layer.
- `examples/resumable_task_examples.cpp` — show patterns that avoid per-task stacks.

## Open Questions / Trade-offs

- **Priority range:** Recommend a configurable compile-time range (e.g., 0..255) rather than a fixed width.
- **Default locking policy:** Should priority inheritance be default or opt-in? Recommend opt-in to keep minimal overhead.

## Deliverables

- Replace this file with the expanded spec (this change).
- Add minimal example(s) and a desktop test harness under `examples/`.
