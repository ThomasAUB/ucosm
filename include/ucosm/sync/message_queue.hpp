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

#include <stddef.h>
#include <type_traits>
#include "uatom.hpp"
#include "memory_order.hpp"

namespace ucosm {

    /**
     * @brief Lock-free, wait-free single producer single consumer queue.
     * Send from one context only, receive from one other context only.
     *
     * @tparam T Message type (must be trivially copyable)
     * @tparam Size Number of messages the queue holds, a power of 2
     */
    template<typename T, size_t Size>
    class MessageQueue {
        static_assert(std::is_trivially_copyable<T>::value,
            "Message type must be trivially copyable");
        static_assert(Size > 0 && (Size & (Size - 1)) == 0,
            "Size must be a power of 2");

    public:
        using message_t = T;

        static constexpr size_t capacity = Size;

        /**
         * @brief Try to send a message (non-blocking). Producer side.
         * @param message Message to send
         * @return true if message was queued, false if queue is full
         */
        bool trySend(const T& message) noexcept {
            const size_t write = mWriteIndex.load(std::memory_order_relaxed);

            if (write - mCachedReadIndex == Size) {
                // looks full : see how far the consumer has got since
                mCachedReadIndex = detail::loadAcquire(mReadIndex);
                if (write - mCachedReadIndex == Size) {
                    return false;
                }
            }

            mBuffer[write & mask] = message;

            detail::storeRelease(mWriteIndex, write + 1);
            return true;
        }

        /**
         * @brief Send as many messages as fit (non-blocking). Producer side.
         * @param messages Messages to send, in order
         * @param count Number of messages
         * @return Number of messages queued, from the start of messages
         */
        size_t trySend(const T* messages, size_t count) noexcept {
            const size_t write = mWriteIndex.load(std::memory_order_relaxed);

            size_t space = Size - (write - mCachedReadIndex);

            if (space < count) {
                mCachedReadIndex = detail::loadAcquire(mReadIndex);
                space = Size - (write - mCachedReadIndex);
                if (space < count) {
                    count = space;
                }
            }

            if (count == 0) {
                return 0;
            }

            const size_t start = write & mask;
            const size_t first = (count < Size - start) ? count : Size - start;
            copy(mBuffer + start, messages, first);
            copy(mBuffer, messages + first, count - first);

            detail::storeRelease(mWriteIndex, write + count);
            return count;
        }

        /**
         * @brief Try to receive a message (non-blocking). Consumer side.
         * @param message Reference to store received message
         * @return true if message was received, false if queue is empty
         */
        bool tryReceive(T& message) noexcept {
            const size_t read = mReadIndex.load(std::memory_order_relaxed);

            if (read == mCachedWriteIndex) {
                // looks empty : see how far the producer has got since
                mCachedWriteIndex = detail::loadAcquire(mWriteIndex);
                if (read == mCachedWriteIndex) {
                    return false;
                }
            }

            message = mBuffer[read & mask];

            detail::storeRelease(mReadIndex, read + 1);
            return true;
        }

        /**
         * @brief Receive as many messages as available, up to a maximum
         * (non-blocking). Consumer side.
         * @param messages Where to store the received messages, in order
         * @param maxCount Maximum number of messages to receive
         * @return Number of messages received
         */
        size_t tryReceive(T* messages, size_t maxCount) noexcept {
            const size_t read = mReadIndex.load(std::memory_order_relaxed);

            size_t available = mCachedWriteIndex - read;

            if (available < maxCount) {
                mCachedWriteIndex = detail::loadAcquire(mWriteIndex);
                available = mCachedWriteIndex - read;
            }

            const size_t count = (available < maxCount) ? available : maxCount;

            if (count == 0) {
                return 0;
            }

            const size_t start = read & mask;
            const size_t first = (count < Size - start) ? count : Size - start;
            copy(messages, mBuffer + start, first);
            copy(messages + first, mBuffer, count - first);

            detail::storeRelease(mReadIndex, read + count);
            return count;
        }

        /**
         * @brief Check if queue is empty.
         * @return true if no messages available
         */
        bool empty() const noexcept {
            return size() == 0;
        }

        /**
         * @brief Check if queue is full.
         * @return true if no space for new messages
         */
        bool full() const noexcept {
            return size() == Size;
        }

        /**
         * @brief Get approximate number of messages in queue.
         * @return Number of messages (may be stale)
         */
        size_t size() const noexcept {
            // read index first : it never passes the write index
            const size_t read = detail::loadAcquire(mReadIndex);
            const size_t write = detail::loadAcquire(mWriteIndex);
            const size_t count = write - read;
            // both sides may have moved between the two loads
            return (count < Size) ? count : Size;
        }

        /**
         * @brief Drop all queued messages. Consumer side.
         */
        void clear() noexcept {
            mCachedWriteIndex = detail::loadAcquire(mWriteIndex);
            detail::storeRelease(mReadIndex, mCachedWriteIndex);
        }

    private:

        static constexpr size_t mask = Size - 1;

        // cheaper than memcpy for small counts
        static void copy(T* outDst, const T* inSrc, size_t inCount) noexcept {
            for (size_t i = 0; i < inCount; ++i) {
                outDst[i] = inSrc[i];
            }
        }

        // producer side
        uatom::Atomic<size_t> mWriteIndex { 0 };
        size_t mCachedReadIndex = 0;

        // consumer side
        uatom::Atomic<size_t> mReadIndex { 0 };
        size_t mCachedWriteIndex = 0;

        T mBuffer[Size];
    };

}
