#include "tests.hpp"

#include "core/function_scheduler.hpp"
#include "periodic/iperiodic_task.hpp"
#include "periodic/periodic_scheduler.hpp"
#include <chrono>
#include <iostream>

struct TaskContainer {

    template<typename sched_t>
    TaskContainer(sched_t& inSched) {
        mTask.setTask(&TaskContainer::task1);
        mTask.setPeriod(1000);
        inSched.addTask(mTask);
    }

    void task1(ucosm::IPeriodicTask& thisTask) {
        std::cout << "coucou " << thisTask.getPeriod() << std::endl;
        if (mCounter++ == 3) {
            mCounter = 0;
            thisTask.setPeriod(100);
            mTask.setTask(&TaskContainer::task2);
        }
    }

    void task2(ucosm::IPeriodicTask& thisTask) {
        std::cout << "hello " << thisTask.getPeriod() << std::endl;
        if (mCounter++ == 30) {
            mCounter = 0;
            thisTask.setPeriod(1000);
            mTask.setTask(nullptr);
        }
    }

    uint8_t mCounter = 0;
    ucosm::MemberFunctionTask<TaskContainer, ucosm::IPeriodicTask> mTask = *this;

};

void memberTaskTests() {

    using namespace ucosm;

    PeriodicScheduler<ITask<uint8_t>> sched(
        +[] () {
            return static_cast<ucosm::IPeriodicTask::tick_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
                );
        }
    );


    TaskContainer tc(sched);

    while (true) {
        sched.run();
    }

}