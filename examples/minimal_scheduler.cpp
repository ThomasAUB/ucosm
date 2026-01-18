// Minimal example using ucosm::ct descriptors
#include <iostream>
#include "ucosm/ct/tasks.hpp"
#include "ucosm/ct/resources.hpp"
#include "ucosm/ct/system.hpp"

struct SensorTag {};
struct ProcTag {};

void sensor_handler() { std::cout << "sensor\n"; }
void proc_handler() { std::cout << "proc\n"; }

constexpr auto sensor_res = ucosm::ct::make_resource<SensorTag, int, ucosm::ct::access_t::Read>(0);
constexpr auto t_sensor = ucosm::ct::make_task<SensorTag, 10>(sensor_handler, sensor_res);
constexpr auto t_proc   = ucosm::ct::make_task<ProcTag, 5>(proc_handler);

int main() {
    auto &sys = ucosm::ct::system();
    sys.init();
    sys.start();
    // Register two example tasks
    sys.register_task(0, 10, &sensor_handler);
    sys.register_task(1, 5, &proc_handler);

    // Notify both and run scheduler a few times
    sys.interrupt_notify(0);
    sys.interrupt_notify(1);

    // schedule will pick the higher-priority (lower numeric) task first
    sys.schedule();
    sys.schedule();
    return 0;
}
