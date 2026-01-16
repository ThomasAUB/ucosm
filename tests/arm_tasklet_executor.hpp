/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2026 Thomas AUBERT                                                *
 *                                                                                 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy    *
 * of this software and associated documentation files (the "Software"), to deal   *
 * in the Software without restriction, including without limitation the rights    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is           *
 * furnished to do so, subject to the following conditions:                        *
 *                                                                                 *
 * The above copyright notice and this permission notice shall be included in all  *
 * copies or substantial portions of the Software.                                 *
 *                                                                                 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 * SOFTWARE.                                                                       *
 *                                                                                 *
 * github : https://github.com/ThomasAUB/ucosm                                     *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include <cstdint>

#include "ucosm/tasklet/tasklet_scheduler.hpp"

inline void request_low_priority_execution();

inline void install_low_priority_handler(void (*handler)(void*), void* ctx);

inline void suspend_low_priority_execution();

inline void resume_low_priority_execution();

inline constexpr ucosm::TaskletBackend tasklet_backend {
    request_low_priority_execution,
    install_low_priority_handler,
    suspend_low_priority_execution,
    resume_low_priority_execution
};

namespace detail {
    inline void (*g_arm_handler)(void*) = nullptr;
    inline void* g_arm_ctx = nullptr;
    inline std::uint32_t g_saved_basepri = 0;
    inline std::uint32_t g_saved_primask = 0;
    inline std::uint32_t g_suspend_count = 0;

    inline void invoke_deferred_handler() {
        auto* handler = g_arm_handler;
        auto* ctx = g_arm_ctx;
        if (handler) {
            handler(ctx);
        }
    }

    inline constexpr bool has_pendsv =
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
        true;
#else
        false;
#endif

    inline constexpr bool has_basepri =
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
        true;
#else
        false;
#endif

    inline constexpr bool has_primask =
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
        true;
#else
        false;
#endif
}

extern "C" void UCOSM_DeferredHandler_Weak() __attribute__((weak));

inline void request_low_priority_execution() {
    if constexpr (detail::has_pendsv) {
        constexpr std::uint32_t ICSR = 0xE000ED04u;
        constexpr std::uint32_t PENDSVSET = 1u << 28;
        auto* const icsr = reinterpret_cast<volatile std::uint32_t*>(ICSR);
        *icsr = PENDSVSET;
    }
    else {
        UCOSM_DeferredHandler_Weak();
    }
}

inline void install_low_priority_handler(void (*handler)(void*), void* ctx) {
    detail::g_arm_handler = handler;
    detail::g_arm_ctx = ctx;
}

inline void suspend_low_priority_execution() {
    if constexpr (detail::has_basepri) {
        if (detail::g_suspend_count == 0) {
            std::uint32_t current_basepri = 0;
            __asm volatile ("mrs %0, basepri" : "=r" (current_basepri) :: "memory");
            detail::g_saved_basepri = current_basepri;

#if defined(SCB)
            const auto pendsv_priority = static_cast<std::uint32_t>(SCB->SHPR[10]);
#else
            const std::uint32_t pendsv_priority = 0xFFu;
#endif
            __asm volatile ("msr basepri, %0" :: "r" (pendsv_priority) : "memory");
        }
        ++detail::g_suspend_count;
        return;
    }

    if constexpr (detail::has_primask) {
        if (detail::g_suspend_count == 0) {
            std::uint32_t current_primask = 0;
            __asm volatile ("mrs %0, primask" : "=r" (current_primask) :: "memory");
            detail::g_saved_primask = current_primask;
            __asm volatile ("cpsid i" ::: "memory");
        }
        ++detail::g_suspend_count;
    }
}

inline void resume_low_priority_execution() {
    if (detail::g_suspend_count == 0) {
        return;
    }

    --detail::g_suspend_count;

    if (detail::g_suspend_count != 0) {
        return;
    }

    if constexpr (detail::has_basepri) {
        __asm volatile ("msr basepri, %0" :: "r" (detail::g_saved_basepri) : "memory");
        return;
    }

    if constexpr (detail::has_primask) {
        __asm volatile ("msr primask, %0" :: "r" (detail::g_saved_primask) : "memory");
    }
}

extern "C" inline void UCOSM_DeferredHandler_Weak() {
    detail::invoke_deferred_handler();
}
