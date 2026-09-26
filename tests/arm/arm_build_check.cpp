// Firmware-shaped use of the schedulers, linked for a Cortex-M core in CI but
// never run.

#include "arm_tasklet_executor.hpp"

#include "ucosm/cfs/cfs_scheduler.hpp"
#include "ucosm/periodic/periodic_scheduler.hpp"
#include "ucosm/rt/rt_inter_task.hpp"
#include "ucosm/rt/rt_scheduler.hpp"

namespace {

    constexpr ucosm::event_id_t exti_event = 0;

    volatile std::uint32_t g_sink = 0;

    struct PeriodicTasklet : ucosm::ITasklet {
        void run() override { g_sink = g_sink + 1; }
    };

    struct EventTasklet : ucosm::ITasklet {
        void run() override { g_sink = g_sink + 2; }
    };

    struct BlinkTask : ucosm::IPeriodicTask {
        void run() override { g_sink = g_sink + 3; }
    };

    struct FairTask : ucosm::ICFSTask {
        void run() override { g_sink = g_sink + 4; }
    };

    ucosm::TaskletScheduler<2> g_tasklets { tasklet_backend };

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
    g_tasklets.signalEvent(exti_event);
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

    ucosm::CFSScheduler<> cfs(get_tick);
    FairTask fair;
    cfs.addTask(fair);

    for (;;) {
        periodic.run();
        cfs.run();
    }
}
