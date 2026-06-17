#pragma once

namespace ucosm {

    template<typename tick_t>
    constexpr inline tick_t makeDeadline(tick_t inBase, tick_t inDelay) {
        return static_cast<tick_t>(inBase + inDelay);
    }

    template<typename tick_t>
    constexpr inline tick_t getDeadlineDelay(tick_t inNow, tick_t inDeadline) {
        return static_cast<tick_t>(inDeadline - inNow);
    }

    template<typename tick_t>
    constexpr inline bool isDeadlineDue(tick_t inCursor, tick_t inDeadline, tick_t inNow) {
        return getDeadlineDelay(inCursor, inDeadline) <= getDeadlineDelay(inCursor, inNow);
    }

}