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

#include <new>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ucosm {

    /**
     * @brief Task running a lambda, function or member function, stored inline.
     * An empty task removes itself when run.
     *
     * @tparam task_t Task implementation
     */
    template<typename task_t>
    struct CallableTask final : task_t {

        CallableTask() = default;

        template<typename callable_t,
            typename = std::enable_if_t<
            std::is_invocable_r_v<void, callable_t> &&
            !std::is_same_v<std::decay_t<callable_t>, CallableTask>>
            >
            CallableTask(callable_t&& c) noexcept(std::is_nothrow_constructible_v<std::decay_t<callable_t>, callable_t&&>) {
            using decayed = std::decay_t<callable_t>;
            static_assert(
                sizeof(decayed) <= storage_size,
                "Callable object too large for internal storage."
                );
            static_assert(
                alignof(decayed) <= storage_align,
                "Callable object alignment too strict for internal storage."
                );
            static_assert(
                std::is_nothrow_move_constructible_v<decayed>,
                "Callable object must be nothrow move constructible."
                );
            ::new(mStorage) decayed(std::forward<callable_t>(c));
            mInvoke = &invoke_impl<decayed>;
            mOps = &ops_table<decayed>;
        }

        template<typename T>
        CallableTask(void(T::* f)(), T& obj)
            : CallableTask([&obj, f] () { (obj.*f)(); }) {}

        template<typename T>
        CallableTask(void(T::* f)(), T* obj)
            : CallableTask([obj, f] () { (obj->*f)(); }) {}

        // the task_t base, scheduler state, is never copied nor moved
        CallableTask(const CallableTask& other) noexcept : task_t() {
            copyFrom(other);
        }

        CallableTask(CallableTask&& other) noexcept : task_t() {
            moveFrom(other);
        }

        ~CallableTask() {
            destroy();
        }

        CallableTask& operator=(CallableTask&& other) noexcept {
            if (this != &other) {
                destroy();
                moveFrom(other);
            }
            return *this;
        }

        CallableTask& operator=(const CallableTask& other) noexcept {
            if (this != &other) {
                destroy();
                copyFrom(other);
            }
            return *this;
        }

    private:

        void run() override {
            mInvoke(mStorage, *this);
        }

        using invoke_t = void(*)(std::byte*, task_t&) noexcept;

        // invoke is kept out of the table, in the object : run() then
        // costs one load before the jump instead of two dependent ones
        struct Operations {
            void (*move)(std::byte*, std::byte*) noexcept;
            void (*copy)(const std::byte*, std::byte*) noexcept;
            void (*destroy)(std::byte*) noexcept;
        };

        template<typename callable_t>
        static void invoke_impl(std::byte* storage, task_t&) noexcept {
            auto* callable = reinterpret_cast<callable_t*>(storage);
            (*callable)();
        }

        template<typename callable_t>
        static void move_impl(std::byte* src, std::byte* dst) noexcept {
            auto* src_callable = reinterpret_cast<callable_t*>(src);
            ::new(dst) callable_t(std::move(*src_callable));
            if constexpr (!std::is_trivially_destructible_v<callable_t>) {
                src_callable->~callable_t();
            }
        }

        template<typename callable_t>
        static void destroy_impl(std::byte* storage) noexcept {
            auto* callable = reinterpret_cast<callable_t*>(storage);
            callable->~callable_t();
        }

        template<typename callable_t>
        static void copy_impl(const std::byte* src, std::byte* dst) noexcept {
            const auto* src_callable = reinterpret_cast<const callable_t*>(src);
            ::new(dst) callable_t(*src_callable);
        }

        template<typename callable_t>
        static constexpr Operations ops_table = {
            &move_impl<callable_t>,
            std::is_copy_constructible_v<callable_t> ? &copy_impl<callable_t> : nullptr,
            std::is_trivially_destructible_v<callable_t> ? nullptr : &destroy_impl<callable_t>
        };

        static void invoke_empty(std::byte*, task_t& self) noexcept {
            self.removeTask();
        }

        static constexpr Operations empty_ops = {
            nullptr,
            nullptr,
            nullptr
        };

        void reset() {
            mInvoke = &invoke_empty;
            mOps = &empty_ops;
        }

        void destroy() {
            if (mOps->destroy) {
                mOps->destroy(mStorage);
            }
        }

        // a callable that can't be copied leaves this task empty
        void copyFrom(const CallableTask& other) {
            if (other.mOps->copy) {
                other.mOps->copy(other.mStorage, this->mStorage);
                mInvoke = other.mInvoke;
                mOps = other.mOps;
            }
            else {
                reset();
            }
        }

        void moveFrom(CallableTask& other) {
            if (other.mOps->move) {
                other.mOps->move(other.mStorage, this->mStorage);
                mInvoke = other.mInvoke;
                mOps = other.mOps;
            }
            else {
                reset();
            }
            other.reset();
        }

        static constexpr size_t storage_size = 4 * sizeof(uintptr_t);
        static constexpr size_t storage_align = alignof(std::max_align_t);

        // mInvoke first, on the vptr's cache line : the only member run() reads
        invoke_t mInvoke = &invoke_empty;
        const Operations* mOps = &empty_ops;
        alignas(storage_align) std::byte mStorage[storage_size];

    };

}