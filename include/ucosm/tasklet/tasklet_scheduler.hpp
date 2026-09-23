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

#include "uatom.hpp"
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
        //
        // One of this and poll() is required : between them they are the only
        // things that make a sleeping task run. A platform on a one-shot timer
        // implements this; a platform on a periodic tick calls poll() from its
        // tick interrupt and leaves this null.
        using schedule_next_wakeup_t = void(*)(bool inHasDeadline, tick_t inDeadline);

        // Reads the platform's clock, in the same unit as the task periods.
        // Required : the scheduler keeps no clock of its own, and calls this
        // wherever it needs the time - several times per run() pass, so it
        // should be cheap. A free running counter read is the intended shape;
        // a counter incremented by a periodic tick interrupt does just as
        // well, and is what a platform driving the scheduler through poll()
        // would return.
        using get_tick_t = tick_t(*)();

        // listed first because it is the only one that must be provided
        get_tick_t getTick = nullptr;

        request_tasklet_execution_t requestTaskletExecution = nullptr;
        handler_installer_t installHandler = nullptr;
        execution_hook_t suspendExecution = nullptr;
        execution_hook_t resumeExecution = nullptr;
        schedule_next_wakeup_t scheduleNextWakeup = nullptr;

    };

    static_assert(uatom::Atomic<tick_t>::is_always_lock_free, "Atomic will be slow");
    static_assert(uatom::Atomic<bool>::is_always_lock_free, "Atomic will be slow");

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

        // Unschedules a task, from whichever list it sits in, and refreshes
        // the next deadline so that a platform driven by scheduleNextWakeup
        // stops waiting for a task that is gone.
        //
        // ITask::removeTask() does the same unlinking but without the
        // ExecutionLock : calling it from outside the task's own run() races
        // the list walk this scheduler does from its dispatch context. Go
        // through here instead.
        void removeTask(ITasklet& inTask);

        // A scheduler is itself a task, so it may be nested in another one and
        // unscheduled from it. Re-exposed because the overload above would
        // otherwise hide it.
        using ITask<uint8_t>::removeTask;

        // Tells the scheduler that time may have moved on, and wakes it if a
        // deadline has come round. Called from a periodic tick (ISR or
        // thread), which is also expected to be what moves the clock that
        // backend getTick reads : this only looks at that clock, it does not
        // advance anything.
        //
        // A platform on a one-shot timer implements backend scheduleNextWakeup
        // instead and never calls this. One of the two is required.
        void poll();

        // current time, read from the backend's clock
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

        // Sorts a task into a list on its plain rank. Used for the lists
        // ranked by priority - the run list and the interrupt lists - where
        // the rank is a value to compare, not a point in time.
        static void insertSort(task_list_t& inList, itask_t& inTask);

        static void mergeSortedLists(task_list_t& ioList, task_list_t& inList);

        void pushReadyInterruptTasks(task_list_t& ioList);

        void pushReadyTimerTasks(task_list_t& ioList);

        // Sorts a task into the timer list, wherever it was before.
        //
        // Deadlines are cyclic, so the list is ordered by the delay separating
        // each task from the cursor rather than by the raw rank : ordering on
        // the rank would put a deadline that has wrapped past zero in front of
        // the cursor, which is the one place getNextTask() never looks.
        //
        // The cursor is therefore always the head of the list, at a delay of
        // zero from itself. Called under an ExecutionLock.
        void insertTimerLocked(itask_t& inTask);

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
        uatom::Atomic<bool> mHasTimerTask { false };
        uatom::Atomic<tick_t> mNextTimer { 0 };
        uatom::Atomic<tick_t> mCursorRank { 0 };

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
    void TaskletScheduler<interrupt_count>::insertTimerLocked(itask_t& inTask) {

        const auto cursor = this->mCursorTask.getRank();
        const auto delay = getDeadlineDelay(cursor, inTask.getRank());

        // Walked from the cursor rather than from the head of the list :
        // everything the cursor has already gone past belongs to the round
        // being retired, and a deadline armed now always comes after it.
        task_list_t::iterator it(&this->mCursorTask);
        ++it;

        const auto endIt = mTimerList.end();

        while (it != endIt && getDeadlineDelay(cursor, it->getRank()) <= delay) {
            ++it;
        }

        // insert_before(end()) appends, so the last slot needs no special case.
        mTimerList.insert_before(it, inTask);
    }

    // called from background and foreground tasks
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::armTimerLocked(ITasklet& inTask, tick_t inDelay) {
        inTask.setRank(makeDeadline(now(), inDelay));
        insertTimerLocked(inTask);
        updateNextTimerLocked();
    }

    // called from background and foreground tasks
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::removeTask(ITasklet& inTask) {

        ExecutionLock guard(*this);

        inTask.ITasklet::removeTask();

        // The task may have been the one the platform is waiting for : tell it
        // what is left, rather than letting it wake up for a deadline that no
        // longer belongs to anyone.
        updateNextTimerLocked();
    }

    // called from ISR
    template<interrupt_id_t interrupt_count>
    void TaskletScheduler<interrupt_count>::poll() {

        if (!mHasTimerTask.load(std::memory_order_acquire)) {
            // nothing sleeping, so no deadline to come round
            return;
        }

        const auto next = mNextTimer.load(std::memory_order_acquire);
        const auto cursor = mCursorRank.load(std::memory_order_acquire);

        if (isDeadlineDue(cursor, next, now())) {
            requestExecution();
        }

    }

    // called from ISR, foregreound and background (anywhere)
    template<interrupt_id_t interrupt_count>
    tick_t TaskletScheduler<interrupt_count>::now() const {
        return mBackend.getTick();
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
            // Captured before the rank gets overwritten for run-list
            // ordering below : run() anchors the next deadline on this
            // value rather than on dispatch time, see run().
            pendTask.setScheduledDeadline(node.getRank());
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

            // Anchor for a task that switches from waitingForInterrupt to
            // sleeping inside run() below : it was never due at a deadline,
            // so its first period counts from the moment it became ready
            // instead, the same way a timer task's counts from the deadline
            // it was due at - see pushReadyTimerTasks().
            const auto readyTick = now();

            // execute the run list's tasks
            while (!runList.empty()) {

                auto& t = static_cast<ITasklet&>(runList.front());

                const bool wasSleeping = t.isSleeping();

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

                    if (!wasSleeping) {
                        // Just configured itself with setPeriod() from a
                        // waitingForInterrupt state : it has no due-deadline
                        // to anchor on, unlike a task that was already
                        // sleeping and keeps whatever pushReadyTimerTasks
                        // stamped it with.
                        t.setScheduledDeadline(readyTick);
                    }

                    const auto period = t.getPeriod();

                    auto deadline = makeDeadline(t.getScheduledDeadline(), period);

                    // If the task took longer to run than its own period,
                    // the computed deadline already lies in the past. Left
                    // as is, it would sort to the far end of the timer list
                    // and never come due again, or make a backend fire its
                    // one-shot wake-up immediately, over and over. Detecting
                    // that case below and re-arming from now instead keeps
                    // the scheduler running.
                    const auto after = now();

                    if (isDeadlinePassed(after, deadline)) {
                        deadline = makeDeadline(after, period);
                    }

                    t.setRank(deadline);
                    insertTimerLocked(t);
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
                    // re-read, so that the time the tasks just spent running
                    // counts towards the next deadline being due
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
#if (defined(__clang__) || defined(__GNUC__)) && (!defined(__arm__) || defined(__ARM_FEATURE_CLZ))
            return static_cast<uint8_t>(__builtin_ctz(v));
#elif defined(__arm__)
            // no CLZ on ARMv6-M : isolate the lowest set bit and look it up
            // with a de Bruijn sequence rather than calling libgcc's __ctzsi2
            static constexpr uint8_t lookup[32] = {
                0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
                31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
            };
            return lookup[((v & (0u - v)) * 0x077CB531u) >> 27];
#else
            uint8_t idx = 0;
            while ((v & 1u) == 0u) { v >>= 1; ++idx; }
            return idx;
#endif
        }

        static constexpr uint8_t storage_size = static_cast<uint8_t>((size + 31) / 32);
        uatom::Atomic<uint32_t> mStorage[storage_size] {};
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