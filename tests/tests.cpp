#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tests.hpp"


TEST_CASE("ucosm tests") {
    periodicTaskTests();
    cfsTaskTests();
}
