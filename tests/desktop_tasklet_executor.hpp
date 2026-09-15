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

#include <condition_variable>
#include <mutex>
#include <thread>

#include "ucosm/tasklet/tasklet_scheduler.hpp"

inline void request_low_priority_execution();

inline void install_low_priority_handler(void (*handler)(void*), void* ctx);

inline void suspend_low_priority_execution();

inline void resume_low_priority_execution();

// Desktop model of the ARM backend (see arm_tasklet_executor.hpp).
//
// The two mechanisms it emulates are deliberately kept independent, exactly as
// they are on target :
//
//  - request_low_priority_execution() models setting the PendSV pending bit : a
//    register write that never blocks, and stays valid while the handler is
//    masked. It must remain callable from an ISR-like context even when
//    execution is suspended.
//
//  - suspend / resume model raising and restoring BASEPRI : they defer the
//    *execution* of the handler, they do not gate the publication of a request.
//
// g_mutex below is only the short-lived lock protecting this state, it is not
// the suspension itself : suspension is g_suspend_count / g_suspend_owner, so a
// suspended section never blocks a concurrent request.
namespace detail {

    inline std::mutex g_mutex;

    // wakes the worker when a request becomes runnable
    inline std::condition_variable g_worker_cv;

    // wakes threads waiting to take the suspension
    inline std::condition_variable g_suspend_cv;

    inline bool g_request_pending = false;
    inline bool g_stop_requested = false;

    inline void (*g_user_handler)(void*) = nullptr;
    inline void* g_user_ctx = nullptr;

    // suspension state, shared between threads : a nesting count plus the
    // thread currently owning it, mirroring g_suspend_count on target.
    inline unsigned g_suspend_count = 0;
    inline std::thread::id g_suspend_owner;

    // set while the worker is inside the user handler, so that a thread asking
    // for suspension waits for the in-flight handler to finish : the equivalent
    // of masking a handler that is already running to completion.
    inline bool g_handler_running = false;
    inline std::thread::id g_handler_thread;

    inline std::thread g_worker_thread;

    inline void thread_main() {

        std::unique_lock<std::mutex> lock(g_mutex);

        while (true) {

            g_worker_cv.wait(
                lock,
                [] { return g_stop_requested || (g_request_pending && g_suspend_count == 0); }
            );

            if (g_stop_requested) {
                break;
            }

            g_request_pending = false;

            auto* const handler = g_user_handler;
            auto* const ctx = g_user_ctx;

            g_handler_running = true;
            g_handler_thread = std::this_thread::get_id();

            // The handler takes the execution lock reentrantly, so it must run
            // with g_mutex released.
            lock.unlock();
            if (handler) {
                handler(ctx);
            }
            lock.lock();

            g_handler_running = false;
            g_handler_thread = std::thread::id {};

            g_suspend_cv.notify_all();
        }
    }
}

inline void request_low_priority_execution() {
    {
        // Publishing the request takes g_mutex only for the store, never the
        // suspension : the worker evaluates its predicate under the same lock,
        // so it cannot read the flag as false and then miss the notification.
        std::lock_guard<std::mutex> lock(detail::g_mutex);
        detail::g_request_pending = true;
    }
    detail::g_worker_cv.notify_one();
}

inline constexpr ucosm::TaskletBackend tasklet_backend {
    request_low_priority_execution,
    install_low_priority_handler,
    suspend_low_priority_execution,
    resume_low_priority_execution
};

inline void suspend_low_priority_execution() {

    std::unique_lock<std::mutex> lock(detail::g_mutex);

    const auto self = std::this_thread::get_id();

    if (detail::g_suspend_count > 0 && detail::g_suspend_owner == self) {
        // nested suspension from the same thread
        ++detail::g_suspend_count;
        return;
    }

    detail::g_suspend_cv.wait(
        lock,
        [&] {
            return detail::g_suspend_count == 0 &&
                (!detail::g_handler_running || detail::g_handler_thread == self);
        }
    );

    detail::g_suspend_count = 1;
    detail::g_suspend_owner = self;
}

inline void resume_low_priority_execution() {

    {
        std::lock_guard<std::mutex> lock(detail::g_mutex);

        if (detail::g_suspend_count == 0 ||
            detail::g_suspend_owner != std::this_thread::get_id()) {
            return;
        }

        if (--detail::g_suspend_count != 0) {
            return;
        }

        detail::g_suspend_owner = std::thread::id {};
    }

    detail::g_worker_cv.notify_one();
    detail::g_suspend_cv.notify_all();
}

inline void stop_low_priority_execution() {
    {
        std::lock_guard<std::mutex> lock(detail::g_mutex);
        detail::g_stop_requested = true;
        detail::g_request_pending = false;
        detail::g_user_handler = nullptr;
        detail::g_user_ctx = nullptr;
    }
    detail::g_worker_cv.notify_all();
    if (detail::g_worker_thread.joinable()) {
        detail::g_worker_thread.join();
    }
}

inline void install_low_priority_handler(void (*handler)(void*), void* ctx) {

    if (handler == nullptr) {
        stop_low_priority_execution();
        return;
    }

    std::lock_guard<std::mutex> lock(detail::g_mutex);

    detail::g_user_handler = handler;
    detail::g_user_ctx = ctx;

    if (!detail::g_worker_thread.joinable()) {
        detail::g_stop_requested = false;
        detail::g_worker_thread = std::thread(detail::thread_main);
    }
}
