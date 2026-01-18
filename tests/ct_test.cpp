
#include "tests.hpp"
#include "doctest.h"
#include "ucosm/ct/system.hpp"
#include <iostream>

void task0() {
    //while (true) {
    std::cout << "coucou from task 0" << std::endl;
    //ucosm::yield();
//}
}
void task1() {
    std::cout << "coucou from task 1" << std::endl;
}
void task2() {
    std::cout << "coucou from task 2" << std::endl;
}

TEST_CASE("ct system skeleton basic operations") {

    using namespace ucosm;

    using pipeline1_t =
        Pipeline<
        7, // prio
        Hook<0>,
        Job<task0>,
        Job<task1>
        >;

    using pipeline2_t =
        Pipeline<
        5, // prio
        Hook<17>,
        Job<task2>
        >;

    System<
        pipeline1_t,
        pipeline2_t
    > system;


    //system.signalHook(0);
    system.signalHook(17);
    system.run();

}
