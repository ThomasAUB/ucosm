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

#include "context.hpp"
#include <cstring>
#include <cstdlib>

namespace ucosm {

    // Current context being executed (for switching)
    static thread_local Context* g_current_context = nullptr;

    // Trampoline function to start a coroutine
    static void coroutineTrampoline() {
        if (g_current_context && g_current_context->entry_point) {
            g_current_context->entry_point(g_current_context->arg);
        }
        // If we reach here, the coroutine finished - this should not happen
        // as coroutines should always yield before finishing
        std::abort();
    }

    void contextSwitch(Context* from, Context* to) {
        // Save current context
        if (setjmp(from->buffer) == 0) {
            // Jump to target context
            g_current_context = to;
            
            if (!to->initialized) {
                // First time switching to this context - start the coroutine
                to->initialized = true;
                coroutineTrampoline();
            } else {
                // Resume the coroutine
                longjmp(to->buffer, 1);
            }
        }
        // We return here when this context is switched back to
    }

    void initializeContext(Context* context, void* stack_base, std::size_t stack_size,
                          void (*entry_point)(void*), void* arg) {
        // Initialize the context
        std::memset(&context->buffer, 0, sizeof(context->buffer));
        context->initialized = false;
        context->entry_point = entry_point;
        context->arg = arg;
        context->stack_base = stack_base;
        context->stack_size = stack_size;
        
        // Note: setjmp/longjmp uses the current stack, so we don't need to
        // set up a separate stack. The stack parameters are stored for reference
        // but not actively used in this implementation.
    }

} // namespace ucosm