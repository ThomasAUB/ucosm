// Firmware-shaped use of the schedulers, linked for a Cortex-M core in CI but
// never run.

#include "arm_tasklet_executor.hpp"

#include "ucosm/periodic/periodic_scheduler.hpp"
#include "ucosm/sync/shared_variable.hpp"
#include "ucosm/tasklet/tasklet_queue.hpp"

namespace {

    constexpr ucosm::event_id_t exti_event = 0;

    volatile std::uint32_t g_sink = 0;

    struct PeriodicTasklet : ucosm::ITasklet {
        void run() override { g_sink = g_sink + 1; }
    };

    ucosm::TaskletScheduler<2> g_tasklets { tasklet_backend };

    // filled by the EXTI interrupt, drained by the event tasklet
    ucosm::TaskletQueue<std::uint32_t, 8, 2> g_extiQueue { g_tasklets, exti_event };

    ucosm::SharedVariable<std::uint32_t> g_lastExti;

    struct EventTasklet : ucosm::ITasklet {
        void run() override {
            std::uint32_t value;
            while (g_extiQueue.tryReceive(value)) {
                g_lastExti.store(value);
            }
        }
    };

    struct BlinkTask : ucosm::IPeriodicTask {
        void run() override { g_sink = g_sink + 3; }
    };

    PeriodicTasklet g_periodicTasklet;
    EventTasklet g_eventTasklet;

}

extern "C" void SysTick_Handler() {
    tick_interrupt(g_tasklets);
}

extern "C" void PendSV_Handler() {
    detail::invoke_deferred_handler();
}

extern "C" void EXTI0_IRQHandler() {
    g_extiQueue.trySend(get_tick());
}

int main() {

    g_periodicTasklet.setPeriod(10);
    g_tasklets.addTask(g_periodicTasklet);

    g_eventTasklet.waitForEvent(exti_event);
    g_tasklets.addTask(g_eventTasklet);

    ucosm::PeriodicScheduler<> periodic(get_tick);
    BlinkTask blink;
    blink.setPeriod(500);
    periodic.addTask(blink);

    for (;;) {
        periodic.run();
        g_sink = g_sink + g_lastExti.load();
    }
}
