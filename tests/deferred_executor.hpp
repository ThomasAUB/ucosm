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
#include <condition_variable>
#include <mutex>
#include <thread>

#include "ucosm/tasklet/tasklet_scheduler.hpp"

inline void request_low_priority_execution();

inline void install_low_priority_handler(void (*handler)(void*), void* ctx);

inline void suspend_low_priority_execution();

inline void resume_low_priority_execution();

namespace detail {
    inline std::atomic<bool> g_request_pending { false };
    inline std::recursive_mutex g_mutex;
    inline std::condition_variable_any g_cv_any;
    inline void (*g_user_handler)(void*) = nullptr;
    inline void* g_user_ctx = nullptr;
    inline std::thread g_worker_thread;
    inline std::atomic<bool> g_stop_requested { false };
    inline thread_local int g_lock_depth = 0;

    inline void thread_main() {
        std::unique_lock<std::recursive_mutex> lk(g_mutex);
        while (!g_stop_requested.load(std::memory_order_acquire)) {
            g_cv_any.wait(lk, [] { return g_request_pending.load(std::memory_order_acquire) || g_stop_requested.load(); });

            if (g_stop_requested.load()) {
                break;
            }

            g_request_pending.store(false, std::memory_order_release);

            auto* handler = g_user_handler;
            auto* ctx = g_user_ctx;
            if (handler) {
                handler(ctx);
            }
        }
    }
}

inline void request_low_priority_execution() {
    detail::g_request_pending.store(true, std::memory_order_release);
    detail::g_cv_any.notify_one();
}

inline constexpr ucosm::TaskletBackend tasklet_backend {
    request_low_priority_execution,
    install_low_priority_handler,
    suspend_low_priority_execution,
    resume_low_priority_execution
};

inline void stop_low_priority_execution() {
    {
        std::lock_guard<std::recursive_mutex> lk(detail::g_mutex);
        detail::g_stop_requested.store(true, std::memory_order_release);
        detail::g_request_pending.store(false, std::memory_order_release);
        detail::g_user_handler = nullptr;
        detail::g_user_ctx = nullptr;
    }
    detail::g_cv_any.notify_one();
    if (detail::g_worker_thread.joinable()) {
        detail::g_worker_thread.join();
    }
}

inline void install_low_priority_handler(void (*handler)(void*), void* ctx) {
    if (handler == nullptr) {
        stop_low_priority_execution();
        return;
    }

    std::lock_guard<std::recursive_mutex> lk(detail::g_mutex);
    detail::g_user_handler = handler;
    detail::g_user_ctx = ctx;

    if (!detail::g_worker_thread.joinable()) {
        detail::g_stop_requested.store(false, std::memory_order_release);
        detail::g_worker_thread = std::thread(detail::thread_main);
    }
}

inline void resume_low_priority_execution() {
    if (detail::g_lock_depth == 0) {
        return;
    }
    --detail::g_lock_depth;
    detail::g_mutex.unlock();
}

inline void suspend_low_priority_execution() {
    detail::g_mutex.lock();
    ++detail::g_lock_depth;
}
