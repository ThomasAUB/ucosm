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
        constexpr void noopHook() {}
        constexpr void noopHandlerInstaller(void (*)(void*), void*) {}
        constexpr void noopScheduleNextWakeup(bool, tick_t) {}

        constexpr void fillDefaultHooks(TaskletBackend& ioBackend) {
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

    struct TaskletScheduler;

    static_assert(uatom::Atomic<TaskletScheduler*>::is_always_lock_free, "Atomic will be slow");

    /**
     * @brief Event tasklets wait for, owned by the code that signals it.
     *
     * An event is identified by its address : events of independent modules
     * can't collide, and their number isn't bounded by the scheduler.
     * It binds to the scheduler of its first waiter, and is released once no
     * task waits for it anymore. Signals sent while unbound are dropped.
     *
     * signal() takes no critical section.
     */
    struct TaskletEvent : ulink::Node<TaskletEvent> {

        constexpr TaskletEvent() = default;

        /**
         * @brief Destroy the event object. The tasks still waiting for it are
         * unscheduled and left unconfigured.
         * @note Must not run while one of its waiters runs.
         */
        ~TaskletEvent();

        TaskletEvent(const TaskletEvent&) = delete;
        TaskletEvent& operator=(const TaskletEvent&) = delete;

        /**
         * @brief Wakes the tasks waiting for the event. ISR safe.
         * Signals are coalesced until the waiters run.
         */
        void signal();

    private:

        friend struct TaskletScheduler;

        template<typename>
        friend class ulink::List;

        // the links belong to the scheduler's event list
        using ulink::Node<TaskletEvent>::remove;
        using ulink::Node<TaskletEvent>::isLinked;

        // written under the scheduler's ExecutionLock, read by signal()
        uatom::Atomic<TaskletScheduler*> mScheduler { nullptr };
        // set by signal(), consumed by the scheduler's run()
        uatom::Atomic<bool> mPending { false };
        // sorted by priority, accessed under the scheduler's ExecutionLock
        ulink::List<ITask<priority_t>> mWaiters;
    };

    /**
     * @brief Scheduler of tasklets woken by timers or events, run from a
     * pended dispatch context.
     */
    struct TaskletScheduler : IScheduler<ITasklet, ITask<uint8_t>> {

        /**
         * @brief Construct a new tasklet scheduler object.
         * Constant-initializable from a constexpr backend without installHandler,
         * e.g. a static placeholder usable before any dynamic initialization.
         *
         * @param inBackend Platform hooks, copied by the scheduler.
         */
        constexpr TaskletScheduler(const TaskletBackend& inBackend) :
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
         * @brief Destroy the tasklet scheduler object, uninstalling its handler
         * and releasing its events.
         */
        ~TaskletScheduler();

        /**
         * @brief Adds a task to the scheduler. Lower priority values run first.
         * A timer task waits for a full period before its first execution.
         *
         * @param inTask Task instance.
         * @return true if the task was successfully added.
         * @return false if the task is unconfigured or waits for an event
         * bound to another scheduler.
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

    protected:

        friend struct TaskletEvent;

        using itask_t = ITask<priority_t>;
        using task_list_t = ulink::List<itask_t>;
        using event_list_t = ulink::List<TaskletEvent>;
        using base_t = IScheduler<ITasklet, ITask<uint8_t>>;

        // called from the dispatch context
        void run() override;

        // Sorts a task by plain rank, for the priority-ranked lists.
        static void insertSort(task_list_t& inList, itask_t& inTask);

        static void mergeSortedLists(task_list_t& ioList, task_list_t& inList);

        void pushReadyEventTasks(task_list_t& ioList);

        void pushReadyTimerTasks(task_list_t& ioList, tick_t inNow);

        // Queues a task on the event it waits for, binding the event if
        // needed. Called under an ExecutionLock.
        bool waitForEventLocked(ITasklet& inTask);

        // Unbinds an event and unlinks its waiters. Called under an ExecutionLock.
        void releaseEventLocked(TaskletEvent& inEvent);

        // Called by TaskletEvent::signal(), from any context.
        void notifyPendingEvent();

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

        task_list_t& timerList() { return base_t::mTasks; }

        static task_list_t& waiters(TaskletEvent& inEvent) { return inEvent.mWaiters; }

        // set from any context, consumed by run() : some event may be pending
        uatom::Atomic<bool> mHasPendingEvents { false };
        uatom::Atomic<bool> mHasTimerTask { false };
        uatom::Atomic<tick_t> mNextTimer { 0 };
        // mirrors mCursor for poll(), which may run from an ISR
        uatom::Atomic<tick_t> mPublishedCursor { 0 };

        TaskletBackend mBackend;

        // last published deadline, only accessed under an ExecutionLock
        bool mNotifiedHasTimer = false;
        tick_t mNotifiedDeadline = 0;

        // bound events, only accessed under an ExecutionLock
        event_list_t mEvents;
    };


    // ===== TaskletEvent ===== //

    inline TaskletEvent::~TaskletEvent() {

        auto* scheduler = mScheduler.load(std::memory_order_relaxed);

        if (!scheduler) {
            return;
        }

        TaskletScheduler::ExecutionLock guard(*scheduler);

        for (auto& t : mWaiters) {
            // waiting again would reach a dead event
            static_cast<ITasklet&>(t).dispose();
        }

        scheduler->releaseEventLocked(*this);
    }

    inline void TaskletEvent::signal() {

        // relaxed : the scheduler outlives the binding, nothing else is read
        auto* scheduler = mScheduler.load(std::memory_order_relaxed);

        if (!scheduler) {
            // no waiter
            return;
        }

        // release : pairs with the acquire in pushReadyEventTasks(), so that
        // the woken tasks see what was written before the signal
        mPending.store(true, std::memory_order_release);

        scheduler->notifyPendingEvent();
    }


    // ===== TaskletScheduler ===== //

    inline TaskletScheduler::~TaskletScheduler() {

        mBackend.installHandler(nullptr, nullptr);

        // no ExecutionLock : the backend may already be unusable
        while (!mEvents.empty()) {
            releaseEventLocked(*mEvents.begin());
        }
    }

    inline bool TaskletScheduler::addTask(ITasklet& inTask) {

        ExecutionLock guard(*this);

        if (inTask.isWaitingForTimer()) {
            armTimerLocked(inTask, inTask.getPeriod());
            return true;
        }

        return waitForEventLocked(inTask);
    }

    inline bool TaskletScheduler::addTask(ITasklet& inTask, tick_t inDelay) {

        ExecutionLock guard(*this);

        if (!inTask.isWaitingForTimer()) {
            return false;
        }

        armTimerLocked(inTask, inDelay);

        return true;
    }

    inline bool TaskletScheduler::setDelay(ITasklet& inTask, tick_t inDelay) {

        ExecutionLock guard(*this);

        if (!inTask.isLinked() || !inTask.isWaitingForTimer()) {
            return false;
        }

        armTimerLocked(inTask, inDelay);

        return true;
    }

    inline void TaskletScheduler::armTimerLocked(ITasklet& inTask, tick_t inDelay) {
        inTask.setRank(makeDeadline(now(), inDelay));
        this->insertByDeadline(inTask);
        updateNextTimerLocked();
    }

    inline bool TaskletScheduler::waitForEventLocked(ITasklet& inTask) {

        const auto* waited = inTask.getEvent();

        if (!waited) {
            return false;
        }

        // the waiter list isn't part of the event's observable state
        auto& event = const_cast<TaskletEvent&>(*waited);

        const auto* owner = event.mScheduler.load(std::memory_order_relaxed);

        if (owner != this) {

            if (owner) {
                // waited for on another scheduler
                return false;
            }

            // signals sent while nobody was waiting are dropped
            event.mPending.store(false, std::memory_order_relaxed);
            mEvents.push_back(event);
            // release : orders the reset above before any signal() binding it
            event.mScheduler.store(this, std::memory_order_release);
        }

        inTask.setRank(inTask.getPriority());
        insertSort(event.mWaiters, inTask);

        return true;
    }

    inline void TaskletScheduler::releaseEventLocked(TaskletEvent& inEvent) {
        inEvent.mScheduler.store(nullptr, std::memory_order_relaxed);
        inEvent.mWaiters.clear();
        inEvent.remove();
    }

    inline void TaskletScheduler::notifyPendingEvent() {
        // release : pairs with the acquire in pushReadyEventTasks()
        mHasPendingEvents.store(true, std::memory_order_release);
        requestExecution();
    }

    inline void TaskletScheduler::removeTask(ITasklet& inTask) {

        ExecutionLock guard(*this);

        inTask.ITasklet::removeTask();

        // the platform may have been waiting for this task
        updateNextTimerLocked();
    }

    inline void TaskletScheduler::poll() {

        // relaxed : a stale read only costs a spurious or one tick late wake-up
        if (!mHasTimerTask.load(std::memory_order_relaxed)) {
            return;
        }

        const auto next = mNextTimer.load(std::memory_order_relaxed);
        const auto cursor = mPublishedCursor.load(std::memory_order_relaxed);

        if (isDeadlineDue(cursor, next, now())) {
            requestExecution();
        }

    }

    inline tick_t TaskletScheduler::now() const {
        return mBackend.getTick();
    }

    inline bool TaskletScheduler::tryGetNextDeadline(tick_t& out) const {
        if (!mHasTimerTask.load(std::memory_order_acquire)) {
            return false;
        }
        out = mNextTimer.load(std::memory_order_relaxed);
        return true;
    }

    // Called with an empty run list : every waiting task is then in its
    // event's list, so an event found without waiters has none left.
    // Releasing it any later, e.g. as soon as its waiters are merged, would
    // drop the signals sent while they run.
    inline void TaskletScheduler::pushReadyEventTasks(task_list_t& ioList) {

        // relaxed load first : on ARMv6-M the exchange masks interrupts
        if (!mHasPendingEvents.load(std::memory_order_relaxed) ||
            !mHasPendingEvents.exchange(false, std::memory_order_acquire)) {
            return;
        }

        for (auto it = mEvents.begin(), endIt = mEvents.end(); it != endIt; ) {

            auto& event = *it;
            ++it;

            const bool isPending =
                event.mPending.load(std::memory_order_relaxed) &&
                event.mPending.exchange(false, std::memory_order_acquire);

            if (event.mWaiters.empty()) {
                // task(s) have been removed
                releaseEventLocked(event);
            }
            else if (isPending) {
                mergeSortedLists(ioList, event.mWaiters);
            }
        }
    }

    inline void TaskletScheduler::pushReadyTimerTasks(task_list_t& ioList, tick_t inNow) {
        auto* task = this->selectReadyTask(inNow);
        if (!task) {
            return;
        }

        const auto cursor = this->mCursor;

        // the first task is known to be due, the loop checks the ones after it
        for (task_list_t::iterator it(task), endIt = timerList().end(); it != endIt; ) {
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

        this->mCursor = inNow;
        // relaxed : poll() is its only reader, and reads it relaxed
        mPublishedCursor.store(inNow, std::memory_order_relaxed);
    }

    inline void TaskletScheduler::run() {

        ExecutionLock guard(*this);

        task_list_t runList;

        for (;;) {

            pushReadyEventTasks(runList);

            // also anchors a task switching from event to timer in its run()
            const auto readyTick = now();

            pushReadyTimerTasks(runList, readyTick);

            while (!runList.empty()) {

                auto& t = static_cast<ITasklet&>(*runList.begin());

                const bool wasWaitingForTimer = t.isWaitingForTimer();

                this->mCurrentTask = &t;

                t.run();

                if (runList.empty() || &*runList.begin() != &t) {
                    // the task removed or rescheduled itself
                    continue;
                }

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
                else if (!waitForEventLocked(t)) {
                    // disposed, or waiting for an event of another scheduler
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

            // a hint only, pushReadyEventTasks() carries the ordering
            if (!(hasTimerDue || mHasPendingEvents.load(std::memory_order_relaxed))) {
                break;
            }
        }

        this->mCurrentTask = nullptr;
    }

    inline void TaskletScheduler::insertSort(task_list_t& inList, itask_t& inTask) {

        const auto rank = inTask.getRank();

        if (inList.empty()) {
            inList.push_front(inTask);
            return;
        }

        const auto frontRank = inList.begin()->getRank();
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

    inline void TaskletScheduler::mergeSortedLists(task_list_t& ioList, task_list_t& inList) {

        if (inList.empty()) {
            return;
        }

        if (ioList.empty()) {
            ioList.splice(ioList.end(), inList);
            return;
        }

        // inList goes after ioList
        if (inList.begin()->getRank() >= ioList.rbegin()->getRank()) {
            ioList.splice(ioList.end(), inList);
            return;
        }

        // inList goes before ioList
        if (inList.rbegin()->getRank() <= ioList.begin()->getRank()) {
            ioList.splice(ioList.begin(), inList);
            return;
        }

        auto it_io = ioList.begin();
        while (!inList.empty()) {
            auto& inNode = *inList.begin();

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

    inline bool TaskletScheduler::updateNextTimerLocked(tick_t& outDeadline) {

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

    inline void TaskletScheduler::requestExecution() const {
        mBackend.requestTaskletExecution();
    }

}
