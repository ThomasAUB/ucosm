// Cycle counts of the tasklet event paths, on a Cortex-M board.
//
// Add this file to any firmware, call ucosm_event_bench() from main(), then
// read g_eventBench with the debugger. Counted with SysTick on the core
// clock, which the bench takes over : run it with the flash wait states and
// cache settings of production. Each figure is the best of 16 runs, minus
// the cost of reading the counter.
//
// The scheduler is dispatched by hand rather than through PendSV, so that
// exception entry and exit, identical for every implementation, stay out of
// the figures.

#include "ucosm/tasklet/tasklet_scheduler.hpp"
#include <stdint.h>

extern "C" {

    struct EventBenchResults {
        uint32_t readOverhead;      // two counter reads, already subtracted below
        uint32_t signal;            // signal() of an idle event
        uint32_t signalCoalesced;   // signal() of an event already pending
        uint32_t dispatchOne;       // dispatch of 1 signaled event out of all
        uint32_t dispatchAll;       // dispatch of every event signaled
        uint32_t dispatchTimer;     // dispatch woken by a timer, no event signaled
    };

    volatile EventBenchResults g_eventBench;

    void ucosm_event_bench();

}

namespace {

    volatile uint32_t& reg(uint32_t inAddress) {
        return *reinterpret_cast<volatile uint32_t*>(inAddress);
    }

    constexpr uint32_t syst_csr = 0xE000E010;
    constexpr uint32_t syst_rvr = 0xE000E014;
    constexpr uint32_t syst_cvr = 0xE000E018;

    ucosm::tick_t g_tick = 0;
    void (*g_handler)(void*) = nullptr;
    void* g_context = nullptr;

    ucosm::tick_t getTick() { return g_tick; }

    void installHandler(void (*inHandler)(void*), void* inContext) {
        g_handler = inHandler;
        g_context = inContext;
    }

    // dispatched by hand
    void requestExecution() {}

    constexpr ucosm::TaskletBackend bench_backend {
        getTick,
        requestExecution,
        installHandler
    };

    struct EmptyTask : ucosm::ITasklet {
        void run() override {}
    };

#ifndef UCOSM_EVENT_BENCH_EVENTS
#define UCOSM_EVENT_BENCH_EVENTS 4
#endif

    constexpr int event_count = UCOSM_EVENT_BENCH_EVENTS;

    void dispatch() { g_handler(g_context); }

    inline uint32_t counter() {
        __asm volatile ("" ::: "memory");
        const uint32_t value = reg(syst_cvr);
        __asm volatile ("" ::: "memory");
        return value;
    }

    // Best of 16 runs of inMeasured, each after inSetup. SysTick counts down.
    template<typename setup_t, typename measured_t>
    uint32_t measure(setup_t&& inSetup, measured_t&& inMeasured, uint32_t inOverhead) {
        uint32_t best = UINT32_MAX;
        for (int i = 0; i < 16; ++i) {
            inSetup();
            const uint32_t start = counter();
            inMeasured();
            const uint32_t end = counter();
            const uint32_t cycles = (start - end) & 0x00FFFFFFu;
            if (cycles < best) {
                best = cycles;
            }
        }
        return best - inOverhead;
    }

}

void ucosm_event_bench() {

    // free-running, core clock, no interrupt
    reg(syst_csr) = 0;
    reg(syst_rvr) = 0x00FFFFFFu;
    reg(syst_cvr) = 0;
    reg(syst_csr) = 0x5u;

    ucosm::TaskletScheduler sched(bench_backend);
    ucosm::TaskletEvent events[event_count];
    EmptyTask eventTasks[event_count];
    EmptyTask timerTask;

    for (int i = 0; i < event_count; ++i) {
        eventTasks[i].setPriority(static_cast<ucosm::priority_t>(i));
        eventTasks[i].waitForEvent(events[i]);
        sched.addTask(eventTasks[i]);
    }

    const auto nothing = [] {};

    const uint32_t overhead = measure(nothing, nothing, 0);
    g_eventBench.readOverhead = overhead;

    g_eventBench.signal = measure(
        dispatch,
        [&] { events[0].signal(); },
        overhead
    );

    g_eventBench.signalCoalesced = measure(
        [&] { dispatch(); events[0].signal(); },
        [&] { events[0].signal(); },
        overhead
    );

    g_eventBench.dispatchOne = measure(
        [&] { events[0].signal(); },
        dispatch,
        overhead
    );

    g_eventBench.dispatchAll = measure(
        [&] {
            for (auto& e : events) {
                e.signal();
            }
        },
        dispatch,
        overhead
    );

    timerTask.setPeriod(1);
    sched.addTask(timerTask);

    g_eventBench.dispatchTimer = measure(
        [&] { ++g_tick; },
        dispatch,
        overhead
    );

    sched.removeTask(timerTask);
}

#ifdef UCOSM_EVENT_BENCH_MAIN
int main() {
    ucosm_event_bench();
    for (;;) {}
}
#endif
