/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2024 Thomas AUBERT                                                *
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

/**
 * @file coroutine.hpp
 * @brief Main header for µCosm coroutine library
 * 
 * This header provides access to the complete µCosm coroutine system including:
 * - Cross-platform context switching (x86_64, ARM64, ARM32)
 * - Zero heap allocation coroutine implementation  
 * - Integration with µCosm scheduler framework
 * - Cooperative multitasking with explicit yield points
 * - Time-based and conditional waiting primitives
 */

// Include the core coroutine components
#include "coroutine/context.hpp"
#include "coroutine/icoroutine.hpp"
#include "coroutine/coroutine_scheduler.hpp"
#include "coroutine/icoroutine_impl.hpp"

/**
 * @namespace ucosm
 * @brief Main namespace for µCosm scheduler and coroutine library
 */
namespace ucosm {

    /**
     * @brief Check if coroutines are supported on the current platform.
     * @return true if coroutines are supported, false otherwise
     */
    constexpr bool isCoroutineSupported() {
        return true;  // setjmp/longjmp is available on all platforms
    }

    /**
     * @brief Get a string describing the coroutine implementation.
     * @return Implementation description string
     */
    constexpr const char* getCoroutineImplementation() {
        return "setjmp/longjmp (portable)";
    }

} // namespace ucosm

/**
 * @example coroutine_examples.cpp
 * This example demonstrates various coroutine usage patterns including:
 * - Simple counting coroutines with yield
 * - Producer-consumer pattern with waiting
 * - State machine implementation
 * - Time-based coordination between coroutines
 */