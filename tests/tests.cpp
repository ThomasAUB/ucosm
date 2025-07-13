#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tests.hpp"

#include <chrono>

uint32_t getMicros() {
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
        );
}

uint32_t getMillis() {
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
        );
}

void waitFor_ms(uint32_t inWait_ms) {
    auto s = getMillis();
    while (getMillis() - s < inWait_ms);
}

TEST_CASE("ucosm tests") {
    periodicTaskTests();
    cfsTaskTests();
    rtTaskTests();
}
