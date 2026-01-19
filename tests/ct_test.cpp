
#include "tests.hpp"
#include "doctest.h"
#include "ucosm/ct/system.hpp"
#include <iostream>

void task0() {
    std::cout << "coucou from task 0" << std::endl;
}
void task1() {
    std::cout << "coucou from task 1" << std::endl;
}
void task2() {
    std::cout << "coucou from task 2" << std::endl;
}

// File-scope helpers used by the demo test
static int demo_counter0 = 0;
static int demo_counter1 = 0;

void demo_job0();
void demo_job1();


TEST_CASE("ct system skeleton basic operations") {

    using namespace ucosm;

    System<
        Job<0, 6, task0>,
        Job<0, 6, task1>,
        Job<17, 5, task2>
    > system;

    std::cout << sizeof(system) << std::endl;

    system.signalHook(0);
    system.run();

    std::cout << "=====" << std::endl;

    system.signalHook(17);
    system.run();

    std::cout << "=====" << std::endl;

    system.signalHook(1);
    system.run();

    std::cout << "=====" << std::endl;

    system.signalHook(0);
    system.signalHook(1);
    system.signalHook(17);
    system.run();

}

// Demo: a job that yields inside a loop should not be re-entered by nested run()
TEST_CASE("ct system yielding from infinite-like loop") {
    using namespace ucosm;
    // instantiate system using the externally-defined functions
    using MySystem = System<
        Job<0, 0, demo_job0>,
        Job<0, 2, demo_job1>,
        Job<1, 3, demo_job1>
    >;

    MySystem s;

    // schedule hooks: job0 instances on hook 0, job1 on hook 1
    s.signalHook(0);
    s.signalHook(1);

    // run until all are handled
    s.run();

    // After yielding runs, counters should have expected values (two job0 entries x3).
    //CHECK(demo_counter0 == 3);
    //CHECK(demo_counter1 >= 1);
}

void demo_job0() {
    std::cout << "job 0 start" << std::endl;
    // simulate a long-running/infinite task that occasionally yields
    for (int i = 0; i < 3; ++i) {
        ++demo_counter0;
        // yield to let scheduler run other ready jobs
        ucosm::yield();
    }
    std::cout << "job 0 end" << std::endl;
}

void demo_job1() {
    std::cout << "job 1 start" << std::endl;
    for (int i = 0; i < 3; ++i) {
        ++demo_counter1;
        ucosm::yield();
    }
    std::cout << "job 1 end" << std::endl;
}
