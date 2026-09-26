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

#include <atomic>

// Memory ordering used by the sync primitives to publish data between
// contexts.
//
// By default they order with acquire / release atomics, which is correct
// everywhere and costs a DMB barrier per access on Cortex-M.
//
// Define UCOSM_SINGLE_CORE to 1 when every context sharing a sync object runs
// on the same core - thread mode and ISRs of a single-core MCU. A core
// observes its own memory accesses in program order, so only the compiler
// must be kept from reordering them : the accesses become relaxed, ordered by
// signal fences that emit no instruction.
//
// Leave it at 0 when a sync object is shared with another core, e.g. between
// the two cores of an RP2040 or an STM32H7 dual-core.
#ifndef UCOSM_SINGLE_CORE
#define UCOSM_SINGLE_CORE 0
#endif

namespace ucosm::detail {

    // Loads a value published by storeRelease() in another context.
    template<typename atomic_t>
    inline auto loadAcquire(const atomic_t& inAtomic) noexcept {
#if UCOSM_SINGLE_CORE
        const auto value = inAtomic.load(std::memory_order_relaxed);
        std::atomic_signal_fence(std::memory_order_acquire);
        return value;
#else
        return inAtomic.load(std::memory_order_acquire);
#endif
    }

    // Publishes a value, and every write made before it, to another context.
    template<typename atomic_t, typename value_t>
    inline void storeRelease(atomic_t& outAtomic, value_t inValue) noexcept {
#if UCOSM_SINGLE_CORE
        std::atomic_signal_fence(std::memory_order_release);
        outAtomic.store(inValue, std::memory_order_relaxed);
#else
        outAtomic.store(inValue, std::memory_order_release);
#endif
    }

}
