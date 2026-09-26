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

#include "ucosm/sync/message_queue.hpp"
#include "tasklet_scheduler.hpp"

namespace ucosm {

    /**
     * @brief Message queue that wakes a tasklet when a message is sent.
     *
     * Pairs a MessageQueue with a scheduler event : each successful
     * trySend() signals the event, so a tasklet waiting for it is run to
     * receive the message. Typically filled from an ISR and drained by the
     * tasklet.
     *
     * Events are coalesced : several sends before the tasklet runs wake it
     * once, so run() must drain the queue, calling tryReceive() until it
     * returns false. A message sent while no tasklet waits for the event
     * does not wake anything, it is received on the next wake-up.
     *
     * Same single producer single consumer rules as MessageQueue.
     *
     * @tparam T Message type (must be trivially copyable)
     * @tparam Size Number of messages the queue holds, a power of 2
     * @tparam event_count Event count of the scheduler
     */
    template<typename T, size_t Size, event_id_t event_count>
    class TaskletQueue {

    public:

        using message_t = T;

        static constexpr size_t capacity = MessageQueue<T, Size>::capacity;

        /**
         * @param inScheduler Scheduler of the tasklet to wake up.
         * @param inEventID Event the tasklet waits for.
         */
        TaskletQueue(TaskletScheduler<event_count>& inScheduler, event_id_t inEventID) :
            mScheduler(inScheduler),
            mEventID(inEventID) {}

        /**
         * @brief Try to send a message and wake the tasklet. Producer side.
         * @return true if message was queued, false if queue is full
         */
        bool trySend(const T& message) noexcept {
            if (!mQueue.trySend(message)) {
                return false;
            }
            mScheduler.signalEvent(mEventID);
            return true;
        }

        /**
         * @brief Send as many messages as fit and wake the tasklet once.
         * Producer side.
         * @return Number of messages queued, from the start of messages
         */
        size_t trySend(const T* messages, size_t count) noexcept {
            const size_t sent = mQueue.trySend(messages, count);
            if (sent) {
                mScheduler.signalEvent(mEventID);
            }
            return sent;
        }

        /**
         * @brief Try to receive a message. Consumer side.
         * @return true if message was received, false if queue is empty
         */
        bool tryReceive(T& message) noexcept { return mQueue.tryReceive(message); }

        /**
         * @brief Receive up to maxCount messages. Consumer side.
         * @return Number of messages received
         */
        size_t tryReceive(T* messages, size_t maxCount) noexcept {
            return mQueue.tryReceive(messages, maxCount);
        }

        bool empty() const { return mQueue.empty(); }

        bool full() const { return mQueue.full(); }

        size_t size() const { return mQueue.size(); }

        // Consumer side.
        void clear() { mQueue.clear(); }

        event_id_t getEventID() const { return mEventID; }

    private:
        MessageQueue<T, Size> mQueue;
        TaskletScheduler<event_count>& mScheduler;
        const event_id_t mEventID;
    };

}
