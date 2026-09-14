#include <iostream>
#include <chrono>
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

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
    // Sleep for most of the duration instead of pure busy-spinning: this
    // runs on the RT scheduler's own ISR thread, and spinning the whole
    // wait starves other threads (timer ISRs, the test's polling loop) of
    // CPU on the limited cores CI runners provide -- enough to blow the RT
    // timing tests' error tolerance under the heavier Debug+sanitizer
    // build. The last millisecond is still spun to keep the wake-up
    // precise (OS sleep can overshoot by more than that).
    constexpr uint32_t spinMargin_ms = 1;
    auto s = getMillis();
    if (inWait_ms > spinMargin_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(inWait_ms - spinMargin_ms));
    }
    while (getMillis() - s < inWait_ms);
}