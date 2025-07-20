#include "tests.hpp"
#include "doctest.h"
#include <iostream>

#include "ucosm/periodic/periodic_scheduler.hpp"
#include "ucosm/periodic/iperiodic_task.hpp"
#include "ucosm/core/callable_task.hpp"

TEST_CASE("Callable task test") {

    int t1Counter = 0;
    int t2Counter = 0;

    ucosm::CallableTask<ucosm::IPeriodicTask> t1(
        [&] () {
            //std::cout << "run " << t1.getPeriod() << std::endl;
            if (++t1Counter == 5) {
                t1.removeTask();
            }
        }
    );

    ucosm::CallableTask<ucosm::IPeriodicTask> t2(
        [&] () {
            //std::cout << "run " << t2.getPeriod() << std::endl;
            if (++t2Counter == 5) {
                t2.removeTask();
            }
        }
    );

    ucosm::CallableTask<ucosm::IPeriodicTask> t3;

    t1.setPeriod(10);
    t2.setPeriod(20);

    ucosm::PeriodicScheduler sched(getMillis);

    sched.addTask(t1);
    sched.addTask(t2);
    sched.addTask(t3);

    struct MemberFunction {

        ucosm::IPeriodicTask& mTask;
        int mCounter = 0;

        MemberFunction(ucosm::IPeriodicTask& t) : mTask(t) {}

        void execute() {
            //std::cout << "run " << mTask.getPeriod() << std::endl;
            if (++mCounter == 5) {
                mTask.removeTask();
            }
        }

    };

    MemberFunction mf(t3);
    t3 = ucosm::CallableTask<ucosm::IPeriodicTask>(&MemberFunction::execute, mf);

    t3.setPeriod(5);

    // insure that an empty callable is safe
    ucosm::CallableTask<ucosm::IPeriodicTask> t4;
    sched.addTask(t4);

    while (!sched.empty()) {
        sched.run();
    }

    CHECK(t1Counter == 5);
    CHECK(t2Counter == 5);
    CHECK(mf.mCounter == 5);

}

TEST_CASE("Callable task advanced tests") {

    // Test empty callable safety
    ucosm::CallableTask<ucosm::IPeriodicTask> emptyTask;
    ucosm::PeriodicScheduler sched(getMillis);
    sched.addTask(emptyTask);

    // Should not crash - empty task removes itself
    for (int i = 0; i < 3 && !sched.empty(); ++i) {
        sched.run();
    }
    CHECK(sched.empty());

    // Test copy and move semantics
    int counter = 0;
    auto lambda = [&counter] () { ++counter; };

    ucosm::CallableTask<ucosm::IPeriodicTask> original(lambda);
    ucosm::CallableTask<ucosm::IPeriodicTask> copied = original;
    ucosm::CallableTask<ucosm::IPeriodicTask> moved = std::move(original);

    // Test that copies work by running them in scheduler
    ucosm::PeriodicScheduler testSched(getMillis);
    copied.setPeriod(1);
    moved.setPeriod(1);

    testSched.addTask(copied);
    testSched.addTask(moved);

    // Run once each
    testSched.run();
    testSched.run();

    CHECK(counter == 2);

    // Test assignment
    ucosm::CallableTask<ucosm::IPeriodicTask> assigned;
    assigned = copied;
    assigned.setPeriod(1);
    testSched.addTask(assigned);
    testSched.run();
    CHECK(counter == 3);
}
