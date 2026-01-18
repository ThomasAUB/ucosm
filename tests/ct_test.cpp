
#include "tests.hpp"
#include "doctest.h"
#include "ucosm/ct/system.hpp"
#include <iostream>

void task0() {
    std::cout << "coucou from task 0" << std::endl;
    //ucosm::yield();
}
void task1() {
    std::cout << "coucou from task 1" << std::endl;
}
void task2() {
    std::cout << "coucou from task 2" << std::endl;
}

TEST_CASE("ct system skeleton basic operations") {

    using namespace ucosm;

    using system = System<
        Task<Guard<0>, task0, 5>,
        Task<Guard<0>, task1, 2>,
        Task<Guard<2>, task2, 8>
    >;

    system sys;
    sys.schedule();

}
