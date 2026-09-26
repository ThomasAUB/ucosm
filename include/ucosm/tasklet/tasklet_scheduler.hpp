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

    /**
     * @brief Platform hooks of a TaskletScheduler.
     * getTick is required, other null hooks are replaced by no-ops.
     */
    struct TaskletBackend final {

        using get_tick_t = tick_t(*)();
        using request_tasklet_execution_t = void(*)();
        using handler_installer_t = void(*)(void(*)(void*), void*);
        using execution_hook_t = void(*)();
        using schedule_next_wakeup_t = void(*)(bool inHasDeadline, tick_t inDeadline);

        /**
         * @brief Returns the current tick. Required, called often so keep it cheap.
         */
        get_tick_t getTick = nullptr;

        /**
         * @brief Pends the dispatch context (e.g. PendSV). May be called from an ISR.
         */
        request_tasklet_execution_t requestTaskletExecution = nullptr;

        /**
         * @brief Installs the handler to call from the dispatch context.
         * Called with nulls at destruction.
         */
        handler_installer_t installHandler = nullptr;

        /**
         * @brief Prevents the dispatch context from running. Calls may nest.
         */
        execution_hook_t suspendExecution = nullptr;

        /**
         * @brief Undoes one suspendExecution() call.
         */
        execution_hook_t resumeExecution = nullptr;

        /**
         * @brief Programs a one-shot timer for the next deadline.
         * Leave null when poll() is called from a periodic tick instead.
         *
         * @param inHasDeadline false when no task waits for the timer.
         * @param inDeadline Absolute tick of the next deadline.
         */
        schedule_next_wakeup_t scheduleNextWakeup = nullptr;

    };

    namespace detail {

        // Stand-ins for null backend hooks, so that call sites need no test.
        inline void noopHook() {}
        inline void noopHandlerInstaller(void (*)(void*), void*) {}
        inline void noopScheduleNextWakeup(bool, tick_t) {}

        inline void fillDefaultHooks(TaskletBackend& ioBackend) {
            if (!ioBackend.requestTaskletExecution) {
                ioBackend.requestTaskletExecution = noopHook;
            }
            if (!ioBackend.installHandler) {
                ioBackend.installHandler = noopHandlerInstaller;
            }
            if (!ioBackend.suspendExecution) {
                ioBackend.suspendExecution = noopHook;
            }
            if (!ioBackend.resumeExecution) {
                ioBackend.resumeExecution = noopHook;
            }
            if (!ioBackend.scheduleNextWakeup) {
                ioBackend.scheduleNextWakeup = noopScheduleNextWakeup;
            }
        }

    }

    static_assert(uatom::Atomic<tick_t>::is_always_lock_free, "Atomic will be slow");
    static_assert(uatom::Atomic<bool>::is_always_lock_free, "Atomic will be slow");

    template<uint8_t size>
    struct Bitset;

    /**
     * @brief Scheduler of tasklets woken by timers or events, run from a
     * pended dispatch context.
     *
     * @tparam event_count Number of events tasks can wait for.
     */
    template<event_id_t event_count>
    struct TaskletScheduler : IScheduler<ITasklet, ITask<uint8_t>> {

        /**
         * @brief Construct a new tasklet scheduler object.
         *
         * @param inBackend Platform hooks, copied by the scheduler.
         */
        TaskletScheduler(const TaskletBackend& inBackend) :
            mBackend(inBackend) {
            detail::fillDefaultHooks(mBackend);
            mBackend.installHandler(
                +[] (void* ctx) {
                    if (ctx) {
                        static_cast<TaskletScheduler*>(ctx)->run();
                    }
                },
                this
            );
        }

        /**
         * @brief Destroy the tasklet scheduler object, uninstalling its handler.
         */
        ~TaskletScheduler() {
            mBackend.installHandler(nullptr, nullptr);
        }

        /**
         * @brief Adds a task to the scheduler. Lower priority values run first.
         * A timer task waits for a full period before its first execution.
         *
         * @param inTask Task instance.
         * @return true if the task was successfully added.
         * @return false if the task is unconfigured or waits for an event
         * this scheduler doesn't have.
         */
        bool addTask(ITasklet& inTask);

        /**
         * @brief Adds a timer task, first run after inDelay instead of a period.
         *
         * @param inTask Task instance.
         * @param inDelay Delay before the first execution.
         * @return true if the task was successfully added.
         * @return false if the task isn't configured with setPeriod().
         */
        bool addTask(ITasklet& inTask, tick_t inDelay);

        /**
         * @brief Sets the delay before the next execution of a scheduled task.
         * The period takes over afterwards. May be called by the task itself.
         *
         * @param inTask Task instance.
         * @param inDelay Delay value.
         * @return true if the task was re-sorted.
         * @return false if the task isn't linked or is waiting for an event.
         */
        bool setDelay(ITasklet& inTask, tick_t inDelay);

        /**
         * @brief Removes a task from the scheduler.
         * Unlike ITask::removeTask(), safe to call from outside the task's run().
         *
         * @param inTask Task instance.
         */
        void removeTask(ITasklet& inTask);

        /**
         * @brief Removes the scheduler from its own parent scheduler.
         */
        using ITask<uint8_t>::removeTask;

        /**
         * @brief Wakes the scheduler if a deadline is due. Call from a periodic
         * tick, unless the backend implements scheduleNextWakeup.
         */
        void poll();

        /**
         * @brief Returns the current time, read from the backend's clock.
         *
         * @return tick_t Current tick.
         */
        tick_t now() const;

        /**
         * @brief Get the next timer deadline, if any.
         *
         * @param out Absolute tick of the next deadline, untouched if none.
         * @return true if a timer is armed.
         * @return false otherwise.
         */
        bool tryGetNextDeadline(tick_t& out) const;

        /**
         * @brief Signals an event to the tasks waiting for it. ISR safe.
         *
         * @param inEventID Event identifier, below event_count.
         */
        void signalEvent(event_id_t inEventID);

    protected:

        using itask_t = ITask<priority_t>;
        using task_list_t = ulink::List<itask_t>;
        using base_t = IScheduler<ITasklet, ITask<uint8_t>>;

        // called from the dispatch context
        void run() override;

        // Sorts a task by plain rank, for the priority-ranked lists.
        static void insertSort(task_list_t& inList, itask_t& inTask);

        static void mergeSortedLists(task_list_t& ioList, task_list_t& inList);

        void pushReadyEventTasks(task_list_t& ioList);

        void pushReadyTimerTasks(task_list_t& ioList, tick_t inNow);

        // Arms a timer task inDelay ticks from now. Called under an ExecutionLock.
        void armTimerLocked(ITasklet& inTask, tick_t inDelay);

        // Publishes the next deadline, returns false when there is none.
        bool updateNextTimerLocked(tick_t& outDeadline);

        void updateNextTimerLocked() {
            tick_t deadline;
            updateNextTimerLocked(deadline);
        }

        void requestExecution() const;

        struct ExecutionLock final {
            explicit ExecutionLock(TaskletScheduler& inScheduler) :
                mScheduler(inScheduler) {
                mScheduler.mBackend.suspendExecution();
            }

            ~ExecutionLock() {
                mScheduler.mBackend.resumeExecution();
            }

            TaskletScheduler& mScheduler;
        };

        ulink::List<ITask<priority_t>>& mTimerList { base_t::mTasks };
        task_list_t mEventTaskLists[event_count];
        // set from ISRs, consumed by run()
        Bitset<event_count> mPendingEvents;
        // written under an ExecutionLock only, read from ISRs
        Bitset<event_count> mSubscribedEvents;
        uatom::Atomic<bool> mHasTimerTask { false };
        uatom::Atomic<tick_t> mNextTimer { 0 };
        uatom::Atomic<tick_t> mCursorRank { 0 };

        // last published deadline, only accessed under an ExecutionLock
        bool mNotifiedHasTimer = false;
        tick_t mNotifiedDeadline = 0;

        TaskletBackend mBackend;
    };


    template<event_id_t event_count>
    bool TaskletScheduler<event_count>::addTask(ITasklet& inTask) {

        ExecutionLock guard(*this);

        if (inTask.isWaitingForTimer()) {
            armTimerLocked(inTask, inTask.getPeriod());
        }
        else if (inTask.isWaitingForEvent()) {

            const auto itID = inTask.getEventID();

            if (itID >= event_count) {
                return false;
            }

            inTask.setRank(inTask.getPriority());
            insertSort(mEventTaskLists[itID], inTask);
            mSubscribedEvents.setLocked(itID);
        }
        else {
            return false;
        }

        return true;
    }

    template<event_id_t event_count>
    bool TaskletScheduler<event_count>::addTask(ITasklet& inTask, tick_t inDelay) {

        ExecutionLock guard(*this);

        if (!inTask.isWaitingForTimer()) {
            return false;
        }

        armTimerLocked(inTask, inDelay);

        return true;
    }

    template<event_id_t event_count>
    bool TaskletScheduler<event_count>::setDelay(ITasklet& inTask, tick_t inDelay) {

        ExecutionLock guard(*this);

        if (!inTask.isLinked() || !inTask.isWaitingForTimer()) {
            return false;
        }

        armTimerLocked(inTask, inDelay);

        return true;
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::armTimerLocked(ITasklet& inTask, tick_t inDelay) {
        inTask.setRank(makeDeadline(now(), inDelay));
        this->insertByDeadline(inTask);
        updateNextTimerLocked();
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::removeTask(ITasklet& inTask) {

        ExecutionLock guard(*this);

        inTask.ITasklet::removeTask();

        // the platform may have been waiting for this task
        updateNextTimerLocked();
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::poll() {

        // relaxed : a stale read only costs a spurious or one tick late wake-up
        if (!mHasTimerTask.load(std::memory_order_relaxed)) {
            return;
        }

        const auto next = mNextTimer.load(std::memory_order_relaxed);
        const auto cursor = mCursorRank.load(std::memory_order_relaxed);

        if (isDeadlineDue(cursor, next, now())) {
            requestExecution();
        }

    }

    template<event_id_t event_count>
    tick_t TaskletScheduler<event_count>::now() const {
        return mBackend.getTick();
    }

    template<event_id_t event_count>
    bool TaskletScheduler<event_count>::tryGetNextDeadline(tick_t& out) const {
        if (!mHasTimerTask.load(std::memory_order_acquire)) {
            return false;
        }
        out = mNextTimer.load(std::memory_order_relaxed);
        return true;
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::signalEvent(event_id_t inEventID) {

        if ((inEventID >= event_count) || !mSubscribedEvents.get(inEventID)) {
            return;
        }

        mPendingEvents.set(inEventID);

        requestExecution();
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::pushReadyEventTasks(task_list_t& ioList) {
        mPendingEvents.consume(
            [&] (uint8_t i) {
                auto& list = mEventTaskLists[i];
                if (list.empty()) {
                    // task(s) have been removed
                    mSubscribedEvents.resetLocked(i);
                }
                else {
                    mergeSortedLists(ioList, list);
                }
            }
        );
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::pushReadyTimerTasks(task_list_t& ioList, tick_t inNow) {
        auto* task = this->selectReadyTask(inNow);
        if (!task) {
            return;
        }

        const auto cursor = this->mCursorTask.getRank();

        // the first task is known to be due, the loop checks the ones after it
        for (task_list_t::iterator it(task), endIt = mTimerList.end(); it != endIt; ) {
            auto& node = *it;
            task_list_t::iterator nextIt = it;
            ++nextIt;

            if (&node != task && !isDeadlineDue(cursor, node.getRank(), inNow)) {
                break;
            }

            auto& pendTask = static_cast<ITasklet&>(node);
            // run() anchors the next deadline on it, not on dispatch time
            pendTask.setScheduledDeadline(node.getRank());
            pendTask.setRank(pendTask.getPriority());
            insertSort(ioList, pendTask);

            it = nextIt;
        }

        this->mCursorTask.setRank(inNow);
        // relaxed : poll() is its only reader, and reads it relaxed
        mCursorRank.store(inNow, std::memory_order_relaxed);
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::run() {

        ExecutionLock guard(*this);

        task_list_t runList;

        for (;;) {

            pushReadyEventTasks(runList);

            // also anchors a task switching from event to timer in its run()
            const auto readyTick = now();

            pushReadyTimerTasks(runList, readyTick);

            while (!runList.empty()) {

                auto& t = static_cast<ITasklet&>(runList.front());

                const bool wasWaitingForTimer = t.isWaitingForTimer();

                this->mCurrentTask = &t;

                t.run();

                if (!t.isLinked() || runList.empty() || &runList.front() != &t) {
                    // the task removed or rescheduled itself
                    continue;
                }

                const auto itID = t.getEventID();

                if (t.isWaitingForTimer()) {

                    if (!wasWaitingForTimer) {
                        // switched from event to timer : no deadline to anchor on
                        t.setScheduledDeadline(readyTick);
                    }

                    const auto period = t.getPeriod();

                    auto deadline = makeDeadline(t.getScheduledDeadline(), period);

                    // overran its period : re-arm from now, a past deadline
                    // would sort to the far end of the list
                    const auto after = now();

                    if (isDeadlinePassed(after, deadline)) {
                        deadline = makeDeadline(after, period);
                    }

                    t.setRank(deadline);
                    this->insertByDeadline(t);
                }
                else if (t.isWaitingForEvent() && itID < event_count) {
                    mSubscribedEvents.setLocked(itID);
                    t.setRank(t.getPriority());
                    insertSort(mEventTaskLists[itID], t);
                }
                else {
                    // disposed, or waiting for an unknown event
                    t.removeTask();
                }
            }

            tick_t nextDeadline;
            const bool hasTimerDue =
                updateNextTimerLocked(nextDeadline) &&
                this->isDue(
                    nextDeadline,
                    // re-read to account for the time spent running tasks
                    now()
                );

            if (!(hasTimerDue || mPendingEvents.any())) {
                break;
            }
        }

        this->mCurrentTask = nullptr;
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::insertSort(task_list_t& inList, itask_t& inTask) {

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

        // walked from the back to keep FIFO order for equal ranks
        auto it = inList.rbegin();
        while (it->getRank() > rank) {
            ++it;
        }

        inList.insert_after(task_list_t::iterator(&*it), inTask);
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::mergeSortedLists(task_list_t& ioList, task_list_t& inList) {

        if (inList.empty()) {
            return;
        }

        if (ioList.empty()) {
            ioList.splice(ioList.end(), inList);
            return;
        }

        // inList goes after ioList
        if (inList.front().getRank() >= ioList.back().getRank()) {
            ioList.splice(ioList.end(), inList);
            return;
        }

        // inList goes before ioList
        if (inList.back().getRank() <= ioList.front().getRank()) {
            ioList.splice(ioList.begin(), inList);
            return;
        }

        auto it_io = ioList.begin();
        while (!inList.empty()) {
            auto& inNode = inList.front();

            while (it_io != ioList.end() && it_io->getRank() <= inNode.getRank()) {
                ++it_io;
            }

            if (it_io == ioList.end()) {
                ioList.splice(ioList.end(), inList);
                break;
            }

            ioList.insert_before(it_io, *inList.begin());
        }
    }

    template<event_id_t event_count>
    bool TaskletScheduler<event_count>::updateNextTimerLocked(tick_t& outDeadline) {

        bool hasTimer = false;
        tick_t deadline = 0;

        if (auto* nextTask = this->getNextTask()) {
            deadline = nextTask->getRank();
            hasTimer = true;
        }

        // only written when changed, to save barriers
        if (hasTimer != mNotifiedHasTimer ||
            (hasTimer && deadline != mNotifiedDeadline)) {
            if (hasTimer) {
                mNextTimer.store(deadline, std::memory_order_relaxed);
            }
            // stored last, with release, for tryGetNextDeadline()
            mHasTimerTask.store(hasTimer, std::memory_order_release);
            mNotifiedHasTimer = hasTimer;
            mNotifiedDeadline = deadline;
            mBackend.scheduleNextWakeup(hasTimer, deadline);
        }

        outDeadline = deadline;
        return hasTimer;
    }

    template<event_id_t event_count>
    void TaskletScheduler<event_count>::requestExecution() const {
        mBackend.requestTaskletExecution();
    }

    // Written either with set() from any context, or with setLocked() /
    // resetLocked() by one writer at a time, never both.
    template<uint8_t size>
    struct Bitset {

        void set(uint8_t i) {
            if (i >= size) { return; }
            const uint8_t idx = static_cast<uint8_t>(i >> 5);
            const uint32_t mask = static_cast<uint32_t>(1u << (i & 0x1F));
            mStorage[idx].fetch_or(mask, std::memory_order_release);
        }

        // relaxed : the bits are a filter, they publish nothing
        void setLocked(uint8_t i) {
            if (i >= size) { return; }
            const uint8_t idx = static_cast<uint8_t>(i >> 5);
            const uint32_t mask = static_cast<uint32_t>(1u << (i & 0x1F));
            const uint32_t v = mStorage[idx].load(std::memory_order_relaxed);
            if ((v & mask) == 0u) {
                mStorage[idx].store(v | mask, std::memory_order_relaxed);
            }
        }

        void resetLocked(uint8_t i) {
            if (i >= size) { return; }
            const uint8_t idx = static_cast<uint8_t>(i >> 5);
            const uint32_t mask = static_cast<uint32_t>(1u << (i & 0x1F));
            const uint32_t v = mStorage[idx].load(std::memory_order_relaxed);
            if ((v & mask) != 0u) {
                mStorage[idx].store(v & ~mask, std::memory_order_relaxed);
            }
        }

        bool get(uint8_t i) const {
            if (i >= size) { return false; }
            const uint32_t mask = static_cast<uint32_t>(1u << (i & 0x1F));
            const uint32_t v = mStorage[i >> 5].load(std::memory_order_relaxed);
            return (v & mask) != 0u;
        }

        // a hint only, consume() carries the ordering
        bool any() const {
            for (uint8_t i = 0; i < storage_size; ++i) {
                if (mStorage[i].load(std::memory_order_relaxed) != 0) {
                    return true;
                }
            }
            return false;
        }

        // Clears the set bits and calls f(index) for each. A bit set meanwhile
        // may be left for the next call, so callers loop while any().
        template<typename F>
        void consume(F&& f) {
            for (uint8_t wordIndex = 0; wordIndex < storage_size; ++wordIndex) {
                if (mStorage[wordIndex].load(std::memory_order_relaxed) == 0u) {
                    continue;
                }
                // pairs with the release in set()
                uint32_t v = mStorage[wordIndex].exchange(0u, std::memory_order_acquire);
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
#if (defined(__clang__) || defined(__GNUC__)) && (!defined(__arm__) || defined(__ARM_FEATURE_CLZ))
            return static_cast<uint8_t>(__builtin_ctz(v));
#elif defined(__arm__)
            // no CLZ on ARMv6-M : de Bruijn lookup instead of libgcc's __ctzsi2
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
        void setLocked(uint8_t) {}
        void resetLocked(uint8_t) {}
        bool get(uint8_t) const { return false; }
        bool any() const { return false; }
        template<typename F>
        void consume(F&&) {}
    };

}