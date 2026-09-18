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
#include "ucosm/core/deadline.hpp"
#include "ucosm/core/ischeduler.hpp"

namespace ucosm {

    struct TaskletBackend final {

        using request_tasklet_execution_t = void(*)();
        using handler_installer_t = void(*)(void(*)(void*), void*);
        using execution_hook_t = void(*)();

        // Called whenever the deadline of the next sleeping task changes,
        // so a platform can reprogram a one-shot hardware timer instead of
        // being driven by a fixed-period tick source. inHasDeadline is false
        // when no task is sleeping (the timer should be stopped); otherwise
        // inDeadline is the absolute tick at which it is next due - use
        // getDeadlineDelay(now(), inDeadline) to turn it into a duration.
        using schedule_next_wakeup_t = void(*)(bool inHasDeadline, tick_t inDeadline);

        // Reads the platform's clock, in the same unit as the task periods.
        //
        // Leaving it null keeps the software clock, which only moves when
        // tick() is called : the scheduler's idea of the current time is then
        // as old as the last call, and a task armed in between is armed
        // relative to that older moment - it becomes due early by however far
        // behind the clock was. A periodic tick bounds that error by its own
        // period, which is what makes it acceptable there.
        //
        // Reading the time from the platform instead removes the error rather
        // than bounding it, at the cost of a call wherever the scheduler needs
        // the time, several times per run() pass. It suits a platform waking
        // the scheduler on its deadlines, where there is no period to bound
        // anything with : scheduleNextWakeup is then required, being the only
        // thing left to make a sleeping task run, and tick() must not be
        // called anymore.
        using read_tick_t = tick_t(*)();

        request_tasklet_execution_t requestTaskletExecution = nullptr;
        handler_installer_t installHandler = nullptr;
        execution_hook_t suspendExecution = nullptr;
        execution_hook_t resumeExecution = nullptr;
        schedule_next_wakeup_t scheduleNextWakeup = nullptr;
        read_tick_t readTick = nullptr;

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

        // Adds a task. Priority is in reverse order: lower values are higher priority.
        // A sleeping task waits for a full period before its first execution.
        bool addTask(ITasklet& inTask) override;

        // Adds a sleeping task whose first execution must not wait for a full
        // period : it is armed inDelay ticks from now, the period taking over
        // afterwards. Returns false for a task that isn't configured with
        // setPeriod(), a delay being meaningless for an interrupt.
        bool addTask(ITasklet& inTask, tick_t inDelay);

        // Sets the delay before the next execution of a task that is already
        // scheduled, and re-sorts it so that the scheduler wakes up for it.
        // Only the next execution is affected, the period takes over
        // afterwards. Like PeriodicScheduler::setDelay the delay is held by
        // the task rank, so it can only be set on a task that is scheduled on
        // the timer : false is returned for a task that isn't linked or that
        // is waiting for an interrupt. A running task may call it on itself to
        // shift its next execution.
        bool setDelay(ITasklet& inTask, tick_t inDelay);

        // called from a periodic tick (ISR or thread) to refresh current time
        // and wake sleeping tasks when their deadline expires.
        // Does nothing when the backend reads the clock itself, the software
        // clock it advances then being unused : the platform wakes the
        // scheduler through scheduleNextWakeup instead.
        void tick(tick_t inc = 1);

        // current time, from the backend's clock when it provides one
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

        // Arms a sleeping task inDelay ticks from now and sorts it into the
        // timer list, wherever it was before. Called under an ExecutionLock.
        void armTimerLocked(ITasklet& inTask, tick_t inDelay);

        bool updateNextTimerLocked();

        void requestExecution() const;

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

        // Last (hasDeadline, deadline) reported to mBackend.scheduleNextWakeup.
        // Only ever touched from updateNextTimerLocked(), which always runs
        // under an ExecutionLock, so these don't need to be atomic.
        bool mNotifiedHasTimer = false;
        tick_t mNotifiedDeadline = 0;

        const TaskletBackend mBackend;
    };


    // called from background and foreground tasks
    template<interrupt_id_t interrupt_count>
    bool TaskletScheduler<interrupt_count>::addTask(ITasklet& inTask) {

        ExecutionLock guard(*this);

        if (inTask.isSleeping()) {
            armTimerLocked(inTask, inTask.getPeriod());
        }
        else if (inTask.isWaitingForInterrupt()) {

            const auto itID = inTask.getInterruptID();

            if (itID >= interrupt_count) {
                return false;
            }

            inTask.setRank(inTask.getPriority());
            insertSort(mBlockedTaskLists[itID], inTask);
            mHasISRTaskID.set(itID);
        }
        else {
            // unconfigured task
            return false;
        }

        return true;
    }

    // called from background and foreground tasks
    template<interrupt_id_t interrupt_count>
    bool TaskletScheduler<interrupt_count>::addTask(ITasklet& inTask, tick_t inDelay) {

        ExecutionLock guard(*this);

        if (!inTask.isSleeping()) {
            return false;
        }

        armTimerLocked(inTask, inDelay);

        return true;
    }

    // called from background and foreground tasks
    template<interrupt_id_t interrupt_count>
    bool TaskletScheduler<interrupt_count>::setDelay(ITasklet& inTask, tick_t inDelay) {

        ExecutionLock guard(*this);

        if (!inTask.isLinked() || !inTask.isSleeping()) {
            // nothing to re-sort : the task isn't scheduled on the timer
            return false;
        }

        armTimerLocked(inTask, inDelay);

        return true;
    }

    // called from background and foreground tasks
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::armTimerLocked(ITasklet& inTask, tick_t inDelay) {
        inTask.setRank(makeDeadline(now(), inDelay));
        insertSort(mTimerList, inTask); // insert after mCursorTask ?
        updateNextTimerLocked();
    }

    // called from ISR
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::tick(tick_t inc) {

        if (mBackend.readTick) {
            // The backend reads the clock, so there is no software clock to
            // advance and nothing here to compare against : the platform is
            // the one deciding when to wake the scheduler, from the deadline
            // it was given.
            return;
        }

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
            requestExecution();
        }

    }

    // called from ISR, foregreound and background (anywhere)
    template<interrupt_id_t interrupt_count>
    tick_t TaskletScheduler<interrupt_count>::now() const {
        if (mBackend.readTick) {
            return mBackend.readTick();
        }
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
        requestExecution();
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

            if (!isDeadlineDue(cursor, node.getRank(), nowTick)) {
                break;
            }

            // The task configuration is left untouched : a task that does
            // not reconfigure itself in run() is re-armed with the same sleep
            // duration, which makes its sleep act as a period.
            auto& pendTask = static_cast<ITasklet&>(node);
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

                if (!t.isLinked() || runList.empty() || &runList.front() != &t) {
                    // the task removed itself, or rescheduled itself through
                    // the scheduler : it already left the run list. The
                    // emptiness check comes first, front() on an empty list
                    // is a fault.
                    continue;
                }

                const auto itID = t.getInterruptID();

                if (t.isSleeping()) {
                    // push into timer list. `current` is the tick sampled
                    // before the task ran, so a late wake-up shifts the next
                    // deadline instead of trying to catch up on missed ones.
                    // A null period is pushed to the next tick : re-arming
                    // the task on the tick it just ran at would make it due
                    // again in this very pass, and the handler would never
                    // return.
                    const auto period = (t.getPeriod() > 0) ? t.getPeriod() : 1;

                    auto deadline = makeDeadline(current, period);

                    // A task whose callable takes longer than its own period
                    // comes back out of run() already overdue, and re-arming
                    // it in the past would make it due again in this very
                    // pass - the handler would keep running it and never
                    // return. Counting the period from the moment it finished
                    // instead drops the deadlines it could not have met, the
                    // same way a late wake-up shifts rather than catches up.
                    // This can only happen when the backend reads the clock :
                    // the software clock does not move while a task runs, so
                    // `after` and `current` are equal there and the deadline
                    // is kept untouched.
                    const auto after = now();

                    if (isDeadlineDue(current, deadline, after)) {
                        deadline = makeDeadline(after, period);
                    }

                    t.setRank(deadline);
                    insertSort(mTimerList, t);
                }
                else if (t.isWaitingForInterrupt() && itID < interrupt_count) {
                    // push into interrupt list
                    mHasISRTaskID.set(itID);
                    t.setRank(t.getPriority());
                    insertSort(mBlockedTaskLists[itID], t);
                }
                else {
                    // The task disposed of itself or is waiting for an
                    // interrupt this scheduler doesn't have : there is no list
                    // to push it into. It must still leave the run list, which
                    // would otherwise keep running it forever.
                    t.removeTask();
                }
            }

            const bool hasTimerDue =
                updateNextTimerLocked() &&
                isDeadlineDue(
                    mCursorRank.load(std::memory_order_acquire),
                    mNextTimer.load(std::memory_order_acquire),
                    // through now(), so that a backend reading the clock
                    // itself sees the time the tasks just spent running
                    now()
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
        inTask.updateRank(inList);
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

        bool hasTimer;
        tick_t deadline = 0;

        if (auto* nextTask = this->getNextTask()) {
            deadline = nextTask->getRank();
            mNextTimer.store(deadline, std::memory_order_release);
            mHasTimerTask.store(true, std::memory_order_release);
            hasTimer = true;
        }
        else {
            mHasTimerTask.store(false, std::memory_order_release);
            hasTimer = false;
        }

        if (mBackend.scheduleNextWakeup &&
            (hasTimer != mNotifiedHasTimer ||
                (hasTimer && deadline != mNotifiedDeadline))) {
            mNotifiedHasTimer = hasTimer;
            mNotifiedDeadline = deadline;
            mBackend.scheduleNextWakeup(hasTimer, deadline);
        }

        return hasTimer;
    }

    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::requestExecution() const {
        if (mBackend.requestTaskletExecution) {
            mBackend.requestTaskletExecution();
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

        // Atomically snapshot all bits and clear them.
        // Each word is individually exchanged, so an ISR can set a bit in a
        // later word between exchanges — that bit will appear in the
        // snapshot AND remain set in *this.  Callers must handle that
        // scenario (e.g. re-checking or tolerating duplicate processing).
        void fetchAndClear(Bitset& dest) {
            for (uint8_t i = 0; i < storage_size; ++i) {
                dest.mStorage[i].store(
                    mStorage[i].exchange(0u, std::memory_order_acq_rel),
                    std::memory_order_release);
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