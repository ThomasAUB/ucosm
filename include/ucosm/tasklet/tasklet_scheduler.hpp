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
#include "itasklet.hpp"
#include "ucosm/core/ischeduler.hpp"

namespace ucosm {

    struct TaskletBackend final {

        using request_tasklet_execution_t = void(*)();
        using handler_installer_t = void(*)(void(*)(void*), void*);
        using execution_hook_t = void(*)();

        request_tasklet_execution_t requestTaskletExecution = nullptr;
        handler_installer_t installHandler = nullptr;
        execution_hook_t suspendExecution = nullptr;
        execution_hook_t resumeExecution = nullptr;

    };

    static_assert(std::atomic<tick_t>::is_always_lock_free, "Atomic will be slow");
    static_assert(std::atomic<bool>::is_always_lock_free, "Atomic will be slow");

    template<uint8_t size>
    struct Bitset;

    /**
     * @brief Pend task scheduler.
     */
    template<interrupt_id_t interrupt_count>
    struct TaskletScheduler : IScheduler<ITasklet, ITask<uint8_t>> {

        TaskletScheduler(const TaskletBackend& inBackend) :
            mBackend(inBackend) {
            if (mBackend.installHandler) {
                mBackend.installHandler(
                    +[] (void* ctx) {
                        if (ctx) {
                            static_cast<TaskletScheduler*>(ctx)->run();
                        }
                    },
                    this
                );
            }
        }

        ~TaskletScheduler() {
            // uninstall handler by passing nulls
            if (mBackend.installHandler) {
                mBackend.installHandler(nullptr, nullptr);
            }
        }

        // adds a task
        bool addTask(ITasklet& inTask);

        // called from a periodic tick (ISR or thread) to refresh current time
        // and wake sleeping tasks when their deadline expires.
        void tick(tick_t inc = 1);

        // current time
        tick_t now() const;

        // return next timer deadline (tick) if any. Returns true and writes
        // the deadline into `out` when a timer is armed, otherwise returns false.
        bool tryGetNextDeadline(tick_t& out) const;

        // called from an ISR
        void signalInterrupt(interrupt_id_t inInterruptID);

    protected:

        using itask_t = ITask<priority_t>;
        using task_list_t = ulink::List<itask_t>;
        using base_t = IScheduler<ITasklet, ITask<uint8_t>>;

        // called from low priority context
        void run() override;

        static void insertSort(task_list_t& inList, itask_t& inTask);

        static void mergeSortedLists(task_list_t& ioList, task_list_t& inList);

        void pushReadyInterruptTasks(task_list_t& ioList);

        void pushReadyTimerTasks(task_list_t& ioList);

        bool updateNextTimerLocked();

        static bool isDeadlineDue(tick_t cursor, tick_t deadline, tick_t nowTick) {
            return static_cast<tick_t>(deadline - cursor) <= static_cast<tick_t>(nowTick - cursor);
        }

        struct ExecutionLock final {
            explicit ExecutionLock(TaskletScheduler& inScheduler) :
                mScheduler(inScheduler) {
                if (mScheduler.mBackend.suspendExecution) {
                    mScheduler.mBackend.suspendExecution();
                }
            }

            ~ExecutionLock() {
                if (mScheduler.mBackend.resumeExecution) {
                    mScheduler.mBackend.resumeExecution();
                }
            }

            TaskletScheduler& mScheduler;
        };

        ulink::List<ITask<priority_t>>& mTimerList { base_t::mTasks };
        task_list_t mBlockedTaskLists[interrupt_count];
        Bitset<interrupt_count> mPendingISR;
        Bitset<interrupt_count> mHasISRTaskID;
        std::atomic<bool> mHasTimerTask { false };
        std::atomic<tick_t> mNextTimer { 0 };
        std::atomic<tick_t> mNow { 0 };
        std::atomic<tick_t> mCursorRank { 0 };

        const TaskletBackend mBackend;
    };


    // called from background and foreground tasks
    template<interrupt_id_t interrupt_count>
    bool TaskletScheduler<interrupt_count>::addTask(ITasklet& inTask) {

        ExecutionLock guard(*this);

        if (inTask.isSleeping()) {
            inTask.setRank(now() + inTask.getSleepDuration());
            insertSort(mTimerList, inTask); // insert after mCursorTask ?
            updateNextTimerLocked();
            return true;
        }

        const auto itID = inTask.getInterruptID();
        if (itID < interrupt_count) {
            inTask.setRank(inTask.getPriority());
            insertSort(mBlockedTaskLists[itID], inTask);
            mHasISRTaskID.set(itID);
            return true;
        }

        return false;
    }

    // called from ISR
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::tick(tick_t inc) {

        // Atomically increment the current time by `inc` and use the
        // resulting value for deadline comparisons
        const tick_t newNow = mNow.fetch_add(inc, std::memory_order_acq_rel) + inc;

        const bool timerArmed = mHasTimerTask.load(std::memory_order_acquire);

        if (!timerArmed) {
            return;
        }

        const auto next = mNextTimer.load(std::memory_order_acquire);
        const auto cursor = mCursorRank.load(std::memory_order_acquire);

        if (isDeadlineDue(cursor, next, newNow)) {
            mBackend.requestTaskletExecution();
        }

    }

    // called from ISR, foregreound and background (anywhere)
    template<interrupt_id_t interrupt_count>
    tick_t TaskletScheduler<interrupt_count>::now() const {
        return mNow.load(std::memory_order_acquire);
    }

    // called from ISR, background, foreground (anywhere)
    template<interrupt_id_t interrupt_count>
    bool TaskletScheduler<interrupt_count>::tryGetNextDeadline(tick_t& out) const {
        if (!mHasTimerTask.load(std::memory_order_acquire)) {
            return false;
        }
        out = mNextTimer.load(std::memory_order_acquire);
        return true;
    }

    // called from ISR, background, foreground (anywhere)
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::signalInterrupt(interrupt_id_t inInterruptID) {

        if ((inInterruptID >= interrupt_count) || !mHasISRTaskID.get(inInterruptID)) {
            return;
        }

        mPendingISR.set(inInterruptID);

        // program low priority function for tasklet execution
        mBackend.requestTaskletExecution();
    }

    // called from foreground
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::pushReadyInterruptTasks(task_list_t& ioList) {
        // pull pending interrupts into run list
        Bitset<interrupt_count> interruptStateCopy;
        mPendingISR.fetchAndClear(interruptStateCopy);

        interruptStateCopy.forEach(
            [&] (uint8_t i) {
                auto& list = mBlockedTaskLists[i];
                if (list.empty()) {
                    // task(s) have been destroyed
                    mHasISRTaskID.reset(i);
                }
                else {
                    mergeSortedLists(ioList, list);
                }
            }
        );
    }

    // called from foreground
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::pushReadyTimerTasks(task_list_t& ioList) {
        // pull expired timers into run list

        auto* task = this->getNextTask();
        if (!task) {
            // no timer tasks in the list
            return;
        }

        const auto nowTick = now();
        const auto cursor = this->mCursorTask.getRank();

        if (!isDeadlineDue(cursor, task->getRank(), nowTick)) {
            // no timer task ready
            return;
        }

        for (task_list_t::iterator it(task), endIt = mTimerList.end(); it != endIt; ) {
            auto& node = *it;
            task_list_t::iterator nextIt = it;
            ++nextIt;

            if (node.getRank() > nowTick) {
                break;
            }

            auto& pendTask = static_cast<ITasklet&>(node);
            pendTask.setSleeping(false);
            pendTask.setRank(pendTask.getPriority());
            insertSort(ioList, pendTask);

            it = nextIt;
        }

        // track the tick value used for deadline comparisons to remain wrap-safe
        this->mCursorTask.setRank(nowTick);
        mCursorRank.store(nowTick, std::memory_order_release);
    }

    // called from foreground
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::run() {

        ExecutionLock guard(*this);

        task_list_t runList;

        for (;;) {

            pushReadyInterruptTasks(runList);
            pushReadyTimerTasks(runList);

            // execute the run list's tasks
            while (!runList.empty()) {

                auto& t = static_cast<ITasklet&>(runList.front());

                const auto current = now();

                t.run();

                if (!t.isLinked()) {
                    continue;
                }

                if (t.isSleeping()) {
                    // push into timer list
                    const auto sleep = t.getSleepDuration();
                    t.setRank(static_cast<tick_t>(current + (sleep > 0 ? sleep : 1)));
                    insertSort(mTimerList, t);
                }
                else {
                    // push into interrupt list
                    const auto itID = t.getInterruptID();
                    if (itID < interrupt_count) {
                        mHasISRTaskID.set(itID);
                        t.setRank(t.getPriority());
                        insertSort(mBlockedTaskLists[itID], t);
                    }
                }
            }

            const bool hasTimerDue =
                updateNextTimerLocked() &&
                isDeadlineDue(
                    mCursorRank.load(std::memory_order_acquire),
                    mNextTimer.load(std::memory_order_acquire),
                    mNow.load(std::memory_order_acquire)
                );

            if (!(mPendingISR.any() || hasTimerDue)) {
                break;
            }
        }
    }

    // called from background and foreground tasks
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::insertSort(task_list_t& inList, itask_t& inTask) {

        const auto rank = inTask.getRank();

        if (inList.empty()) {
            inList.push_front(inTask);
            return;
        }

        const auto frontRank = inList.front().getRank();
        if (rank < frontRank) {
            inList.push_front(inTask);
            return;
        }

        const auto backRank = inList.back().getRank();
        if (rank > backRank) {
            inList.push_back(inTask);
            return;
        }

        inList.push_back(inTask);
        inTask.updateRank();
    }

    // called from foreground task
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::mergeSortedLists(task_list_t& ioList, task_list_t& inList) {

        if (inList.empty()) {
            return;
        }

        if (ioList.empty()) {
            ioList.splice(ioList.end(), inList);
            return;
        }

        // Fast-path: all nodes in `inList` come after `ioList` -> splice to end
        if (inList.front().getRank() >= ioList.back().getRank()) {
            ioList.splice(ioList.end(), inList);
            return;
        }

        // Fast-path: all nodes in `inList` come before `ioList` -> splice to front
        if (inList.back().getRank() <= ioList.front().getRank()) {
            ioList.splice(ioList.begin(), inList);
            return;
        }

        // Merge in linear time by walking both lists and splicing single nodes
        // from `inList` into their correct position in `ioList`.
        auto it_io = ioList.begin();
        while (!inList.empty()) {
            auto& inNode = inList.front();

            // Advance io iterator until it points to the first node
            // with rank > inNode.rank (insert before it).
            while (it_io != ioList.end() && it_io->getRank() <= inNode.getRank()) {
                ++it_io;
            }

            if (it_io == ioList.end()) {
                // Remaining nodes in inList are >= all ioList nodes -> splice the rest
                ioList.splice(ioList.end(), inList);
                break;
            }

            ioList.insert_before(it_io, *inList.begin());
        }
    }

    // called from background and foreground
    template<interrupt_id_t interrupt_count>
    bool TaskletScheduler<interrupt_count>::updateNextTimerLocked() {
        if (auto* nextTask = this->getNextTask()) {
            mNextTimer.store(nextTask->getRank(), std::memory_order_release);
            mHasTimerTask.store(true, std::memory_order_release);
            return true;
        }
        else {
            mHasTimerTask.store(false, std::memory_order_release);
            return false;
        }
    }

    template<uint8_t size>
    struct Bitset {

        void set(uint8_t i) {
            if (i >= size) { return; }
            const uint8_t idx = static_cast<uint8_t>(i >> 5);
            const uint32_t mask = static_cast<uint32_t>(1u << (i & 0x1F));
            mStorage[idx].fetch_or(mask, std::memory_order_acq_rel);
        }

        void reset(uint8_t i) {
            if (i >= size) { return; }
            const uint8_t idx = static_cast<uint8_t>(i >> 5);
            const uint32_t mask = static_cast<uint32_t>(1u << (i & 0x1F));
            mStorage[idx].fetch_and(static_cast<uint32_t>(~mask), std::memory_order_acq_rel);
        }

        bool get(uint8_t i) const {
            if (i >= size) { return false; }
            const uint32_t mask = static_cast<uint32_t>(1u << (i & 0x1F));
            const uint32_t v = mStorage[i >> 5].load(std::memory_order_acquire);
            return (v & mask) != 0u;
        }

        bool any() const {
            for (uint8_t i = 0; i < storage_size; ++i) {
                if (mStorage[i].load(std::memory_order_acquire) != 0) {
                    return true;
                }
            }
            return false;
        }

        // atomically fetch current storage and clear it (set to zero)
        // writes the previous content into 'dest'
        // this avoids races with concurrent ISR writes (which typically use atomic fetch_or()).
        void fetchAndClear(Bitset& dest) {
            for (uint8_t i = 0; i < storage_size; ++i) {
                uint32_t prev = mStorage[i].exchange(0u, std::memory_order_acq_rel);
                dest.mStorage[i].store(prev, std::memory_order_release);
            }
        }

        template<typename F>
        void forEach(F&& f) const {
            for (uint8_t wordIndex = 0; wordIndex < storage_size; ++wordIndex) {
                uint32_t v = mStorage[wordIndex].load(std::memory_order_acquire);
                while (v) {
                    const uint8_t bitOffset = firstSetBit(v);
                    const uint8_t globalIdx = static_cast<uint8_t>((wordIndex << 5) + bitOffset);
                    if (globalIdx >= size) { return; }
                    f(globalIdx);
                    v = v & (v - 1u);
                }
            }
        }

    private:

        static uint8_t firstSetBit(uint32_t v) {
            // use builtin to avoid the loop when available
#if defined(__clang__) || defined(__GNUC__)
            return static_cast<uint8_t>(__builtin_ctz(v));
#else
            uint8_t idx = 0;
            while ((v & 1u) == 0u) { v >>= 1; ++idx; }
            return idx;
#endif
        }

        static constexpr uint8_t storage_size = static_cast<uint8_t>((size + 31) / 32);
        std::atomic<uint32_t> mStorage[storage_size] {};
    };

    template<>
    struct Bitset<0> {
        void set(uint8_t) {}
        void reset(uint8_t) {}
        bool get(uint8_t) const { return false; }
        bool any() const { return false; }
        void fetchAndClear(Bitset& dest) {}
        template<typename F>
        void forEach(F&& f) const {}
    };

}