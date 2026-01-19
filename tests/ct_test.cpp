
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
