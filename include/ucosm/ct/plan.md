# Implementation Plan — Compile-time Task Scheduler (ucosm::ct)

This document defines an actionable implementation plan and strategy to implement the compile-time task scheduler described in `spec.md`.

## Summary

Deliver a small, zero-heap, compile-time-driven scheduler core in C++17 that:

- Allows `constexpr`/template task and resource declarations.
- Performs compile-time validation (dependency cycles, resource conflicts).
- Emits runtime data structures for scheduling decisions.
- Provides a minimal runtime API for initialization, dispatch and ISR notifications.
- Is platform-neutral; platform-specific preemption/context-switch hooks live outside the core.

## Architecture Overview

- `ct::Task` (compile-time descriptor): lightweight descriptor type that records handler, priority, resources.
- `ct::Resource` (compile-time descriptor): describes static resource with access attributes.
- `ct::SystemBuilder` (constexpr builder): accepts task and resource descriptors and performs compile-time analysis to produce `ct::System`.
- `ct::System` (runtime data): static storage for scheduler state and runtime entry points (`init`, `start`, `schedule`, `yield`, `interrupt_notify`).
- `ct::analyzer` (compile-time utilities): graph validation, resource conflict checks, priority inversion detection.
- `platform/` (optional): small adapters that implement preemption/context switch on specific targets (Cortex-M, POSIX threads for desktop demos).

## Milestones

1. Project scaffolding and CI (small): add CI job for host builds and basic tests.
2. Core descriptors (Task/Resource) and builder API (constexpr): minimal API to declare tasks/resources.
3. Compile-time analyzer: detect cycles and basic resource conflicts using constexpr template metaprogramming.
4. System runtime skeleton: define `System` runtime APIs and static storage layout.
5. Examples: minimal desktop example and resumable task example.
6. Platform glue examples: Cortex-M notes and POSIX demo thread integration.
7. Performance tuning and memory footprint reports.

## Detailed Work Plan and Tasks

1) Define core types and DSL (2-3 days)

- Implement lightweight descriptor types: `ct::TaskDesc<NameTag, Priority, Handler, Resources...>` and `ct::ResourceDesc<NameTag, Attributes, Storage&>`.
- Provide convenience creation helpers: `constexpr make_task<Name, Priority>(handler, resources...)` and `constexpr make_resource<Name>(storage, attrs)`.
- Design naming/tagging strategy for readable diagnostics (use `struct` tags rather than string template args where possible to improve compiler messages).

2) Implement the SystemBuilder and compile-time graph builder (3-5 days)

- `constexpr SystemBuilder::add_task(taskDesc)` collects tasks into a type-list.
- Build a compile-time adjacency list and compute topological ordering where possible.
- On cycle detection, produce a clear `static_assert` with task identifiers.

3) Resource model and conflict detection (3-4 days)

- Define resource access attributes: `ReadOnly`, `Shared`, `Exclusive`.
- For each resource, compute which tasks access it and in which mode (at compile time).
- Emit `static_assert` if two tasks require exclusive access but no locking protocol is declared.

4) Runtime System and scheduling decisions (3-5 days)

- Define `ct::System` runtime struct with static storage arrays for ready queues (by priority), task state, and small scheduler metadata.
- Implement `init()`, `start()`, `schedule()` and `yield()` runtime functions. Keep these minimal; they manipulate static arrays only.
- Implement a lock/notify primitive for ISR use: `interrupt_notify(task_id)` which sets task state atomically (document required atomicity semantics).

5) Cooperative / Resumable API (2-3 days)

- Provide small helpers and examples showing how to write resumable tasks using lambdas and state machines (no stack switching).

6) Examples and platform glue (2-4 days)

- Desktop example using a POSIX thread to simulate preemption (or cooperative scheduler loop).
- Cortex-M integration notes showing where to call `schedule()` from PendSV and how to wire `interrupt_notify` from IRQs.

7) Tests and CI (2-3 days)

- Unit tests for `analyzer` using `static_assert`-based checks where possible. For tests that cannot be `static_assert`-only, use compile-time configured test files.
- Host runtime tests to validate scheduling decisions, ISR notify, and cooperative examples.

8) Performance and footprint reporting (1-2 days)

- Add a small `size_report` target to compute object sizes and a minimal example to measure code size and RAM usage.

Total rough estimate: 3–4 weeks for an initial polished v0 with examples and CI; smaller MVP in ~1 week focusing on descriptors, analyzer and basic runtime.

## API Details and Examples

- `template<typename Tag, int Priority, typename Handler, typename... Resources>
  struct TaskDesc;`
- `template<typename Tag, typename Storage, AccessAttrs> struct ResourceDesc;`
- `constexpr auto make_task<Tag, Priority>(handler, resources...) -> TaskDesc<...>`
- `constexpr auto build_system(tasks..., resources...) -> SystemBuilderResult` — performs compile-time checks and returns a `constexpr` system descriptor.

Runtime usage:

- `auto &sys = System::instance();`
- `sys.init(); sys.start();` — `start()` runs scheduler loop on desktop or yields control to the platform layer on MCU.

ISR usage:

- `sys.interrupt_notify(task_id);` — must be callable from ISR. Document which atomic operations are required (e.g., `std::atomic_flag` or platform critical-section).

## Testing Strategy

- Use a mix of compile-time `static_assert` tests (to assert analyzer conclusions) and runtime unit tests for scheduling behaviour.
- Build desktop tests that exercise ordering, preemption simulation, and ISR notifications.
- Add CI that runs tests on host (Linux) and reports results.

## Risk Analysis and Mitigations

- Template complexity and long compile times: mitigate by providing helper macros, split header structure, and incremental development with smaller compile-time checks.
- Poor compiler diagnostics: use `struct` tags and `static_assert` messages to improve errors; provide documentation with examples mapping errors to fixes.
- Platform-specific preemption pitfalls: keep core minimal and supply clear platform adapter examples.

## Performance Considerations

- Minimize template instantiation bloat by centralizing common compile-time utilities and using `constexpr` arrays for static data.
- Keep runtime operations branchless where possible and use small fixed-size arrays instead of dynamic containers.

## Deliverables

- `include/ucosm/ct/*.hpp` — header-only core API and DSL.
- `src/tests/` — host tests for analyzer and runtime.
- `examples/minimal_scheduler.cpp`, `examples/resumable_task_examples.cpp`, `examples/mcu_integration.md`.
- CI: host build + tests + static analysis.
