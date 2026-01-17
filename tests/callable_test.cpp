#include "tests.hpp"
#include "doctest.h"
#include <chrono>
#include <iostream>

#include "ucosm/periodic/periodic_scheduler.hpp"
#include "ucosm/periodic/iperiodic_task.hpp"
#include "ucosm/core/callable_task.hpp"

TEST_CASE("Callable task test") {

    StreamSilencer silence(std::cout);

    int t1Counter = 0;
    int t2Counter = 0;

    ucosm::CallableTask<ucosm::IPeriodicTask> t1;

    t1 = [&] () {
        //std::cout << "run " << t1.getPeriod() << std::endl;
        if (++t1Counter == 5) {
            t1.removeTask();
        }
        };

    ucosm::CallableTask<ucosm::IPeriodicTask> t2;

    t2 = [&] () {
        //std::cout << "run " << t2.getPeriod() << std::endl;
        if (++t2Counter == 5) {
            t2.removeTask();
        }
        };

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

    StreamSilencer silence(std::cout);
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

TEST_CASE("Callable task micro-benchmark") {

    StreamSilencer silence(std::cout);

    using namespace ucosm;

    using clock = std::chrono::high_resolution_clock;
    constexpr int iterations = 200000;

    int callableCounter = 0;
    CallableTask<IPeriodicTask> callable([&] () { ++callableCounter; });

    struct DerivedTask final : IPeriodicTask {
        int counter = 0;
        void run() override { ++counter; }
    } derived;

    auto bench = [&] (auto&& fn) {
        // small warmup to stabilize branch prediction and caches
        for (int i = 0; i < 64; ++i) { fn(); }
        const auto start = clock::now();
        for (int i = 0; i < iterations; ++i) { fn(); }
        const auto end = clock::now();
        return end - start;
        };

    const auto callableDur = bench([&] () { static_cast<IPeriodicTask&>(callable).run(); });
    const auto derivedDur = bench([&] () { derived.run(); });

    const auto callableNs = std::chrono::duration_cast<std::chrono::nanoseconds>(callableDur).count();
    const auto derivedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(derivedDur).count();

    INFO("Callable total ns: ", callableNs);
    INFO("Derived total ns: ", derivedNs);
    INFO("Callable ns/call: ", static_cast<double>(callableNs) / iterations);
    INFO("Derived ns/call: ", static_cast<double>(derivedNs) / iterations);

    CHECK(callableCounter == iterations + 64);
    CHECK(derived.counter == iterations + 64);
}

TEST_CASE("Callable task - assign empty does not crash") {
    StreamSilencer silence(std::cout);
    using namespace ucosm;

    CallableTask<IPeriodicTask> target([] {});

    CallableTask<IPeriodicTask> empty;
    // move-assign from empty: previously set mOps=nullptr, causing
    // the next run() to segfault on mOps->invoke.
    target = std::move(empty);

    PeriodicScheduler sched(getMillis);
    sched.addTask(target);
    sched.run(); // empty_ops::invoke_empty removes the task
    CHECK(sched.empty());

    // copy-assign from empty: same class of bug.
    CallableTask<IPeriodicTask> copyTarget([] {});
    copyTarget = empty;
    sched.addTask(copyTarget);
    sched.run();
    CHECK(sched.empty());
}
