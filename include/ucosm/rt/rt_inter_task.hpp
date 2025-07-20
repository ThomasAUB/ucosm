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

#include <stdint.h>
#include <stddef.h>
#include <atomic>
#include <type_traits>

namespace ucosm {

    /**
     * @brief Lock-free, wait-free message queue for RT inter-task communication.
     *
     * This queue provides safe communication between tasks of different priorities
     * without blocking or dynamic allocation. Works with both PC threads and
     * microcontroller hardware timers.
     *
     * Features:
     * - Single Producer, Single Consumer (SPSC) for maximum performance
     * - Lock-free and wait-free operations
     * - No dynamic memory allocation
     * - Interrupt-safe on microcontrollers
     * - Template-based for type safety
     *
     * @tparam T Message type (must be trivially copyable)
     * @tparam Size Queue capacity (must be power of 2)
     */
    template<typename T, size_t Size>
    class RTMessageQueue {
        static_assert(std::is_trivially_copyable<T>::value,
            "Message type must be trivially copyable");
        static_assert((Size& (Size - 1)) == 0,
            "Size must be power of 2");

    public:
        using message_t = T;
        static constexpr size_t capacity = Size;

        /**
         * @brief Construct queue with all slots empty.
         */
        RTMessageQueue() : mReadIndex(0), mWriteIndex(0) {}

        /**
         * @brief Try to send a message (non-blocking).
         * @param message Message to send
         * @return true if message was queued, false if queue is full
         */
        bool trySend(const T& message) {
            const size_t currentWrite = mWriteIndex.load(std::memory_order_relaxed);
            const size_t nextWrite = (currentWrite + 1) & (Size - 1);

            // Check if queue is full
            if (nextWrite == mReadIndex.load(std::memory_order_acquire)) {
                return false;
            }

            // Store message
            mBuffer[currentWrite] = message;

            // Publish the write
            mWriteIndex.store(nextWrite, std::memory_order_release);
            return true;
        }

        /**
         * @brief Try to receive a message (non-blocking).
         * @param message Reference to store received message
         * @return true if message was received, false if queue is empty
         */
        bool tryReceive(T& message) {
            const size_t currentRead = mReadIndex.load(std::memory_order_relaxed);

            // Check if queue is empty
            if (currentRead == mWriteIndex.load(std::memory_order_acquire)) {
                return false;
            }

            // Load message
            message = mBuffer[currentRead];

            // Publish the read
            mReadIndex.store((currentRead + 1) & (Size - 1), std::memory_order_release);
            return true;
        }

        /**
         * @brief Check if queue is empty.
         * @return true if no messages available
         */
        bool empty() const {
            return mReadIndex.load(std::memory_order_acquire) ==
                mWriteIndex.load(std::memory_order_acquire);
        }

        /**
         * @brief Check if queue is full.
         * @return true if no space for new messages
         */
        bool full() const {
            const size_t nextWrite = (mWriteIndex.load(std::memory_order_acquire) + 1) & (Size - 1);
            return nextWrite == mReadIndex.load(std::memory_order_acquire);
        }

        /**
         * @brief Get approximate number of messages in queue.
         * @return Number of messages (may be stale)
         */
        size_t size() const {
            const size_t write = mWriteIndex.load(std::memory_order_acquire);
            const size_t read = mReadIndex.load(std::memory_order_acquire);
            return (write - read) & (Size - 1);
        }

        /**
         * @brief Clear all messages from queue.
         */
        void clear() {
            mReadIndex.store(mWriteIndex.load(std::memory_order_acquire),
                std::memory_order_release);
        }

    private:
        // Platform-optimized alignment to prevent false sharing
        alignas(std::max_align_t) std::atomic<size_t> mReadIndex;
        alignas(std::max_align_t) std::atomic<size_t> mWriteIndex;
        alignas(std::max_align_t) T mBuffer[Size];
    };

    /**
     * @brief Shared variable for safe inter-task communication.
     *
     * Provides atomic access to a single value with optional versioning
     * to detect updates. Suitable for sensor data, control values, etc.
     *
     * @tparam T Value type (must be trivially copyable and fit in atomic)
     */
    template<typename T>
    class RTSharedVariable {
        static_assert(std::is_trivially_copyable<T>::value,
            "Type must be trivially copyable");
        static_assert(sizeof(T) <= sizeof(uint64_t),
            "Type too large for atomic operations");
        static_assert(std::atomic<T>::is_always_lock_free,
            "Type must be lock-free on this platform (use smaller types on MCU)");

    public:
        /**
         * @brief Construct with initial value.
         * @param initialValue Initial value
         */
        explicit RTSharedVariable(const T& initialValue = T {})
            : mVersion(0), mValue(initialValue) {}

        /**
         * @brief Atomically update the value.
         * @param newValue New value to store
         * @note Version is only incremented if value actually changes
         */
        void store(const T& newValue) {
            T expected = mValue.load(std::memory_order_relaxed);
            while (expected != newValue) {
                if (mValue.compare_exchange_weak(expected, newValue,
                    std::memory_order_release,
                    std::memory_order_relaxed)) {
                    mVersion.fetch_add(1, std::memory_order_release);
                    break;
                }
                // expected is updated by compare_exchange_weak on failure
            }
        }

        /**
         * @brief Atomically read the current value.
         * @return Current value
         */
        T load() const {
            return mValue.load(std::memory_order_acquire);
        }

        /**
         * @brief Read value and version atomically.
         * @param value Reference to store current value
         * @return Current version number
         * @note Ensures consistent version-value pair reading
         */
        uint32_t loadWithVersion(T& value) const {
            uint32_t version1, version2;
            do {
                version1 = mVersion.load(std::memory_order_acquire);
                value = mValue.load(std::memory_order_acquire);
                version2 = mVersion.load(std::memory_order_acquire);
            } while (version1 != version2);
            return version2;
        }

        /**
         * @brief Get current version number.
         * @return Version number (increments on each store)
         */
        uint32_t getVersion() const {
            return mVersion.load(std::memory_order_acquire);
        }

        /**
         * @brief Check if value has changed since last version.
         * @param lastVersion Previous version number
         * @return true if value has been updated
         */
        bool hasChanged(uint32_t lastVersion) const {
            return mVersion.load(std::memory_order_acquire) != lastVersion;
        }

        /**
         * @brief Atomic compare and swap operation
         * @param expected Expected current value (updated on failure)
         * @param desired New value to set if comparison succeeds
         * @return true if swap occurred, false otherwise
         */
        bool compareAndSwap(T& expected, const T& desired) {
            bool result = mValue.compare_exchange_weak(expected, desired,
                std::memory_order_acq_rel);
            if (result) {
                mVersion.fetch_add(1, std::memory_order_acq_rel);
            }
            return result;
        }

    private:
        alignas(std::max_align_t) std::atomic<uint32_t> mVersion;
        alignas(std::max_align_t) std::atomic<T> mValue;
    };

    /**
     * @brief Event flags for task synchronization.
     *
     * Provides a way for tasks to signal events to each other.
     * Each bit represents a different event type.
     */
    class RTEventFlags {
    public:
        using flags_t = uint32_t;

        /**
         * @brief Construct with no flags set.
         */
        RTEventFlags() : mFlags(0) {}

        /**
         * @brief Set one or more event flags.
         * @param flags Flags to set (bitwise OR)
         */
        void setFlags(flags_t flags) {
            mFlags.fetch_or(flags, std::memory_order_release);
        }

        /**
         * @brief Clear one or more event flags.
         * @param flags Flags to clear
         */
        void clearFlags(flags_t flags) {
            mFlags.fetch_and(~flags, std::memory_order_release);
        }

        /**
         * @brief Check if any of the specified flags are set.
         * @param flags Flags to check
         * @return true if any flag is set
         */
        bool testAny(flags_t flags) const {
            return (mFlags.load(std::memory_order_acquire) & flags) != 0;
        }

        /**
         * @brief Check if all of the specified flags are set.
         * @param flags Flags to check
         * @return true if all flags are set
         */
        bool testAll(flags_t flags) const {
            return (mFlags.load(std::memory_order_acquire) & flags) == flags;
        }

        /**
         * @brief Wait for any of the specified flags and clear them.
         * @param flags Flags to wait for
         * @return Flags that were set
         */
        flags_t waitAndClear(flags_t flags) {
            flags_t current;
            do {
                current = mFlags.load(std::memory_order_acquire);
                if ((current & flags) != 0) {
                    // Try to clear the flags atomically
                    if (mFlags.compare_exchange_weak(current, current & ~flags,
                        std::memory_order_release)) {
                        return current & flags;
                    }
                }
            } while ((current & flags) == 0);
            return current & flags;
        }

        /**
         * @brief Wait for any of the specified flags with timeout.
         * @param flags Flags to wait for
         * @param timeout Timeout
         * @return true if flags were set within timeout
         * @note On microcontrollers, this does limited polling to avoid blocking
         */
        bool waitAny(flags_t flags, uint32_t timeout) const {
            for (uint32_t i = 0; i < timeout; ++i) {
                if (testAny(flags)) {
                    return true;
                }
                // Minimal delay - in real MCU this would be hardware-specific
                for (volatile int j = 0; j < 1000; ++j) {} // Simple delay loop
            }
            return false;
        }

        /**
         * @brief Wait for all of the specified flags with timeout.
         * @param flags Flags to wait for
         * @param timeout Timeout
         * @return true if all flags were set within timeout
         * @note On microcontrollers, this does limited polling to avoid blocking
         */
        bool waitAll(flags_t flags, uint32_t timeout) const {
            for (uint32_t i = 0; i < timeout; ++i) {
                if (testAll(flags)) {
                    return true;
                }
                // Minimal delay - in real MCU this would be hardware-specific
                for (volatile int j = 0; j < 1000; ++j) {} // Simple delay loop
            }
            return false;
        }

        /**
         * @brief Get current flag state.
         * @return Current flags
         */
        flags_t getFlags() const {
            return mFlags.load(std::memory_order_acquire);
        }

    private:
        std::atomic<flags_t> mFlags;
    };

}
