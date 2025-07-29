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

#include <cstdint>
#include <cstddef>
#include <csetjmp>

namespace ucosm {

    /**
     * @brief Context structure using standard C setjmp/longjmp for portability.
     */
    struct Context {
        std::jmp_buf buffer;
        bool initialized;
        void (*entry_point)(void*);
        void* arg;
        void* stack_base;
        std::size_t stack_size;
    };

    /**
     * @brief Switch from current context to target context.
     * @param from Pointer to current context (will be saved here)
     * @param to Pointer to target context (will be restored from here)
     */
    void contextSwitch(Context* from, Context* to);

    /**
     * @brief Initialize a context for a new coroutine.
     * @param context Pointer to context to initialize
     * @param stack_base Pointer to base of stack for this context
     * @param stack_size Size of the stack in bytes
     * @param entry_point Function to call when context is first switched to
     * @param arg Argument to pass to entry_point
     */
    void initializeContext(Context* context, void* stack_base, std::size_t stack_size,
                          void (*entry_point)(void*), void* arg);

    /**
     * @brief Get the required stack alignment for the current platform.
     * @return Stack alignment in bytes
     */
    constexpr std::size_t getStackAlignment() {
        return sizeof(void*);  // Align to pointer size
    }

    /**
     * @brief Get the minimum recommended stack size for coroutines.
     * @return Minimum stack size in bytes
     */
    constexpr std::size_t getMinStackSize() {
        return 8192;  // 8KB minimum stack for setjmp/longjmp
    }

} // namespace ucosm