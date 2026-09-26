#include "tests.hpp"
#include "doctest.h"

#include "ucosm/tasklet/tasklet_scheduler.hpp"

// Tests for the two ways a platform drives the scheduler. The clock is always
// the platform's, read through TaskletBackend::getTick - what differs is what
// makes the scheduler look at it :
//
//  - a one-shot timer, reprogrammed through scheduleNextWakeup on whichever
//    deadline comes next, so the platform takes no interrupt in between;
//
//  - a periodic tick, which moves the clock and then calls poll(), leaving
//    scheduleNextWakeup unused.
//
// Everything here is synchronous : the harness below models the platform's
// deferred execution and its one-shot timer with plain flags, so that a test
// can move time and inspect the result without racing anything.

namespace {

    using namespace ucosm;

    // the platform's clock
    tick_t g_now = 0;

    // the platform's deferred execution request, i.e. PendSV's pending bit
    bool g_pending = false;

    void (*g_handler)(void*) = nullptr;
    void* g_context = nullptr;
    int g_suspendCount = 0;

    // the platform's one-shot timer
    bool g_wakeupArmed = false;
    tick_t g_wakeupDeadline = 0;
    int g_wakeupCalls = 0;

    void requestExecution() { g_pending = true; }

    void installHandler(void (*inHandler)(void*), void* inContext) {
        g_handler = inHandler;
        g_context = inContext;
    }

    void suspendExecution() { ++g_suspendCount; }

    void resumeExecution() {
        if (g_suspendCount > 0) { --g_suspendCount; }
    }

    void scheduleNextWakeup(bool inHasDeadline, tick_t inDeadline) {
        g_wakeupArmed = inHasDeadline;
        g_wakeupDeadline = inDeadline;
        ++g_wakeupCalls;
    }

    tick_t getTick() { return g_now; }

    // A platform sleeping between deadlines : it is handed each one through
    // scheduleNextWakeup and arms a one-shot on it.
    const TaskletBackend oneshot_backend {
        getTick,
        requestExecution,
        installHandler,
        suspendExecution,
        resumeExecution,
        scheduleNextWakeup
    };

    // The same platform on a periodic tick : its tick interrupt moves the
    // clock and calls poll(), so it needs no wake-up to be scheduled for it.
    const TaskletBackend periodic_backend {
        getTick,
        requestExecution,
        installHandler,
        suspendExecution,
        resumeExecution,
        nullptr
    };

    void resetHarness() {
        g_now = 0;
        g_pending = false;
        g_handler = nullptr;
        g_context = nullptr;
        g_suspendCount = 0;
        g_wakeupArmed = false;
        g_wakeupDeadline = 0;
        g_wakeupCalls = 0;
    }

    // whether the armed one-shot would have fired by now
    bool isWakeupDue() {
        return g_wakeupArmed &&
            static_cast<int32_t>(g_wakeupDeadline - g_now) <= 0;
    }

    // Runs the deferred handler until nothing is pending, the way PendSV
    // alone would. For the periodic backend, where no one-shot is armed and
    // it is poll() that pends the handler.
    void pumpPending() {
        for (int guard = 0; guard < 100; ++guard) {
            if (!g_pending) {
                return;
            }
            g_pending = false;
            if (g_handler) {
                g_handler(g_context);
            }
        }
        FAIL("the scheduler did not settle");
    }

    // Runs the deferred handler until nothing is pending and no armed wake-up
    // is due, the way PendSV and the timer interrupt would between them.
    void pump() {
        for (int guard = 0; guard < 100; ++guard) {
            if (!g_pending && !isWakeupDue()) {
                return;
            }
            g_pending = false;
            if (g_handler) {
                g_handler(g_context);
            }
        }
        FAIL("the scheduler did not settle");
    }

    // Moves the platform's clock, then lets the scheduler run whatever this
    // made due. For the one-shot backend : the timer it armed is what brings
    // the scheduler back, so nothing else has to be called.
    void advanceClock(tick_t inDelta) {
        g_now += inDelta;
        pump();
    }

    // What a periodic tick interrupt does : move the clock, then let the
    // scheduler look at it. Without the poll() nothing would happen - the
    // scheduler is never told the time on its own.
    template<event_id_t event_count>
    void tickInterrupt(TaskletScheduler<event_count>& inScheduler, tick_t inDelta) {
        g_now += inDelta;
        inScheduler.poll();
        pumpPending();
    }

    struct CountTask : ITasklet {
        void run() override { ++mCount; }
        int mCount = 0;
    };

    struct OneShotTask : ITasklet {
        void run() override {
            ++mCount;
            this->removeTask();
        }
        int mCount = 0;
    };

}

TEST_CASE("TaskletScheduler - one-shot timer : a task armed after an idle stretch is not due early") {

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    // Time passes with nothing scheduled, so nothing wakes the scheduler at
    // all. Reading the clock rather than counting elapsed ticks is what makes
    // this come out right : a scheduler counting ticks would still be at 0.
    g_now = 5'000;

    CountTask task;
    task.setPeriod(500);
    REQUIRE(sched.addTask(task));

    // armed relative to the clock, not to the last moment the scheduler ran
    REQUIRE(g_wakeupArmed);
    CHECK(g_wakeupDeadline == tick_t(5'500));

    advanceClock(499);
    CHECK(task.mCount == 0);

    advanceClock(1);
    CHECK(task.mCount == 1);
}

TEST_CASE("TaskletScheduler - periodic tick : the same clock, looked at by poll()") {

    // The other way round : no wake-up is scheduled for this platform, and it
    // is the tick interrupt calling poll() that brings the scheduler back. The
    // clock is read the same way, so the deadline is the same absolute value.

    resetHarness();
    TaskletScheduler<1> sched(periodic_backend);

    g_now = 5'000;

    CountTask task;
    task.setPeriod(500);
    REQUIRE(sched.addTask(task));

    // armed relative to the clock, as before - but nothing was scheduled,
    // this backend having no one-shot to arm
    tick_t nextDeadline = 0;
    REQUIRE(sched.tryGetNextDeadline(nextDeadline));
    CHECK(nextDeadline == tick_t(5'500));
    CHECK_FALSE(g_wakeupArmed);

    tickInterrupt(sched, 499);
    CHECK(task.mCount == 0);

    tickInterrupt(sched, 1);
    CHECK(task.mCount == 1);
}

TEST_CASE("TaskletScheduler - periodic tick : moving the clock without poll() does nothing") {

    // poll() is the whole of the wake-up path here : the scheduler is never
    // told the time on its own, so a clock that moves behind its back leaves
    // the task waiting, however far past the deadline it goes.

    resetHarness();
    TaskletScheduler<1> sched(periodic_backend);

    CountTask task;
    task.setPeriod(50);
    REQUIRE(sched.addTask(task));

    g_now = 1'000;
    pumpPending();
    CHECK(task.mCount == 0);

    // and one poll() is enough to make it due
    sched.poll();
    pumpPending();
    CHECK(task.mCount == 1);
}

TEST_CASE("TaskletScheduler - one-shot timer : a period is re-armed from the clock") {

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    CountTask task;
    task.setPeriod(50);
    REQUIRE(sched.addTask(task));
    CHECK(g_wakeupDeadline == tick_t(50));

    advanceClock(50);
    CHECK(task.mCount == 1);
    CHECK(g_wakeupDeadline == tick_t(100));

    advanceClock(50);
    CHECK(task.mCount == 2);
    CHECK(g_wakeupDeadline == tick_t(150));

    advanceClock(50);
    CHECK(task.mCount == 3);
    CHECK(g_wakeupDeadline == tick_t(200));
}

TEST_CASE("TaskletScheduler - one-shot timer : dispatch latency does not shift the period") {

    // On real hardware the handler never sees now() land exactly on the
    // deadline : ISR/PendSV latency and list-walk overhead mean a few ticks
    // have already elapsed by the time run() reads the clock. The next
    // deadline must stay anchored on the original schedule (multiples of the
    // period) rather than drifting later by that same latency every pass.

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    CountTask task;
    task.setPeriod(200);
    REQUIRE(sched.addTask(task));
    CHECK(g_wakeupDeadline == tick_t(200));

    constexpr tick_t dispatch_latency = 4;

    // dispatched 4 ticks after its 200 deadline
    advanceClock(200 + dispatch_latency);
    CHECK(task.mCount == 1);
    CHECK(g_wakeupDeadline == tick_t(400));

    // dispatched 4 ticks after its 400 deadline
    advanceClock(200);
    CHECK(task.mCount == 2);
    CHECK(g_wakeupDeadline == tick_t(600));

    // dispatched 4 ticks after its 600 deadline
    advanceClock(200);
    CHECK(task.mCount == 3);
    CHECK(g_wakeupDeadline == tick_t(800));
}

TEST_CASE("TaskletScheduler - one-shot timer : the deadline is the earliest of the tasks") {

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    g_now = 1'000;

    OneShotTask early;
    OneShotTask late;

    early.setPeriod(30);
    late.setPeriod(70);

    REQUIRE(sched.addTask(late));
    CHECK(g_wakeupDeadline == tick_t(1'070));

    // adding a task due sooner re-arms the platform
    REQUIRE(sched.addTask(early));
    CHECK(g_wakeupDeadline == tick_t(1'030));

    advanceClock(30);
    CHECK(early.mCount == 1);
    CHECK(late.mCount == 0);

    // the remaining task is the one armed now
    CHECK(g_wakeupArmed);
    CHECK(g_wakeupDeadline == tick_t(1'070));

    advanceClock(40);
    CHECK(late.mCount == 1);

    // nothing left to wait for : the platform is told to stop its timer
    CHECK_FALSE(g_wakeupArmed);
}

TEST_CASE("TaskletScheduler - one-shot timer : a task slower than its period does not trap run()") {

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    // A task whose callable takes longer than its own period. Counted from the
    // moment the batch started, its next deadline is already behind the clock
    // by the time it returns - re-arming it there would make it due again in
    // the same pass, and run() would never return. The deadlines it could not
    // have met are dropped instead.
    struct SlowTask : ITasklet {
        void run() override {
            ++mCount;
            g_now += 80; // the callable takes longer than the 50 tick period
        }
        int mCount = 0;
    };

    SlowTask task;
    task.setPeriod(50);
    REQUIRE(sched.addTask(task));
    CHECK(g_wakeupDeadline == tick_t(50));

    // run() has to hand control back rather than run the task forever
    advanceClock(50);
    CHECK(task.mCount == 1);

    // ran at 50 and finished at 130, so the next one is a period from there
    CHECK(g_now == tick_t(130));
    REQUIRE(g_wakeupArmed);
    CHECK(g_wakeupDeadline == tick_t(180));

    advanceClock(50);
    CHECK(task.mCount == 2);
    CHECK(g_now == tick_t(260));
    CHECK(g_wakeupDeadline == tick_t(310));
}

TEST_CASE("TaskletScheduler - one-shot timer : a task faster than its period keeps an exact period") {

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    // The counterpart of the test above : as long as the callable fits in the
    // period, the deadline stays counted from the previous one, so the time
    // the task spends running does not accumulate into its period.
    struct BusyTask : ITasklet {
        void run() override {
            ++mCount;
            g_now += 10; // comfortably inside the 50 tick period
        }
        int mCount = 0;
    };

    BusyTask task;
    task.setPeriod(50);
    REQUIRE(sched.addTask(task));

    advanceClock(50);
    CHECK(task.mCount == 1);
    CHECK(g_wakeupDeadline == tick_t(100));

    advanceClock(40); // clock now at 100, the task having eaten 10
    CHECK(task.mCount == 2);
    CHECK(g_wakeupDeadline == tick_t(150));
}

TEST_CASE("TaskletScheduler - one-shot timer : an event task runs without any deadline") {

    resetHarness();
    TaskletScheduler<2> sched(oneshot_backend);

    CountTask task;
    task.waitForEvent(1);
    REQUIRE(sched.addTask(task));

    // nothing waiting for the timer, so the platform's timer stays off
    CHECK_FALSE(g_wakeupArmed);

    g_now = 9'999;

    sched.signalEvent(1);
    pump();
    CHECK(task.mCount == 1);

    sched.signalEvent(1);
    pump();
    CHECK(task.mCount == 2);

    CHECK_FALSE(g_wakeupArmed);
}

TEST_CASE("TaskletScheduler - one-shot timer : a task armed across the tick wrap is not skipped") {

    // Deadlines are cyclic, so around the moment the clock wraps past zero the
    // timer list holds both kinds at once : deadlines that have wrapped,
    // numerically tiny, and the cursor they are ahead of, numerically huge.
    // Ordering them on the raw value rather than on the delay separating them
    // from the cursor files a wrapped deadline in front of it, which is the one
    // place the scheduler never looks for the next task to run.

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    g_now = 0xFFFF'FF00;

    // Runs a hair before the wrap and re-arms past it. That pass is what
    // leaves the list holding a deadline numerically below the cursor.
    CountTask spanning;
    spanning.setPeriod(0xF0);
    REQUIRE(sched.addTask(spanning));
    CHECK(g_wakeupDeadline == tick_t(0xFFFF'FFF0));

    advanceClock(0xF0);
    REQUIRE(spanning.mCount == 1);
    REQUIRE(g_wakeupDeadline == tick_t(0xE0));

    // Armed from there, due before it.
    CountTask early;
    early.setPeriod(0x20);
    REQUIRE(sched.addTask(early));

    REQUIRE(g_wakeupArmed);
    CHECK(g_wakeupDeadline == tick_t(0x10));

    advanceClock(0x20);
    CHECK(early.mCount == 1);
    CHECK(spanning.mCount == 1);

    // and the one that spans the wrap still runs at its own deadline
    advanceClock(0xD0);
    CHECK(spanning.mCount == 2);
}

TEST_CASE("TaskletScheduler - one-shot timer : removeTask drops the deadline it was armed on") {

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    CountTask task;
    task.setPeriod(500);
    REQUIRE(sched.addTask(task));
    REQUIRE(g_wakeupArmed);

    sched.removeTask(task);

    // nothing left to wait for : the platform's timer is told to stop rather
    // than left running for a task that is gone
    CHECK_FALSE(g_wakeupArmed);

    advanceClock(1'000);
    CHECK(task.mCount == 0);
}

TEST_CASE("TaskletScheduler - one-shot timer : a wake-up later than a full period does not strand the task") {

    // The counterpart of the dispatch-latency test above. Anchoring the next
    // deadline on the one that was due only keeps it ahead of the clock while
    // the task is less than a period late. Past that - a platform slow to
    // wake the scheduler, or a clock that jumped - the anchored deadline is
    // already behind, and arming it there is not merely late : the timer list
    // is ordered on the delay from the cursor, so a deadline behind it sorts
    // to the far end and never comes due, while the platform is handed a
    // one-shot that fires the moment it is armed, for ever.

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    CountTask task;
    task.setPeriod(50);
    REQUIRE(sched.addTask(task));
    CHECK(g_wakeupDeadline == tick_t(50));

    // woken at 170, i.e. 120 ticks after the 50 deadline : more than two
    // periods late, so 50 + 50 is no longer a deadline to come
    advanceClock(170);
    CHECK(task.mCount == 1);

    // the missed deadlines are dropped, and the next one counted from here
    REQUIRE(g_wakeupArmed);
    CHECK(g_wakeupDeadline == tick_t(220));

    // and the task keeps running
    advanceClock(50);
    CHECK(task.mCount == 2);
    CHECK(g_wakeupDeadline == tick_t(270));
}

TEST_CASE("TaskletScheduler - periodic tick : a tick jump larger than a period does not strand the task") {

    // Same thing on the periodic backend, where a tick coarser than the
    // period - a tickless idle catching up, or a tick source that stalled -
    // makes the task arrive more than a period late.

    resetHarness();
    TaskletScheduler<1> sched(periodic_backend);

    CountTask task;
    task.setPeriod(10);
    REQUIRE(sched.addTask(task));

    tick_t nextDeadline = 0;
    REQUIRE(sched.tryGetNextDeadline(nextDeadline));
    CHECK(nextDeadline == tick_t(10));

    // straight from 0 to 35, so the 10 deadline is 25 ticks stale
    tickInterrupt(sched, 35);
    CHECK(task.mCount == 1);

    REQUIRE(sched.tryGetNextDeadline(nextDeadline));
    CHECK(nextDeadline == tick_t(45));

    tickInterrupt(sched, 10);
    CHECK(task.mCount == 2);
}

TEST_CASE("TaskletScheduler - an event task that gives itself a period is armed from now") {

    // A task dispatched from an event was never due at a deadline, so it
    // has none to anchor a period on. Anchoring it on whatever it last
    // carried - zero, for a task that never came off the timer list - would
    // arm it far behind the clock : due again in the very same pass, then
    // stranded.

    resetHarness();
    TaskletScheduler<2> sched(periodic_backend);

    struct SwitchTask : ITasklet {
        void run() override {
            ++mCount;
            if (mCount == 1) {
                this->setPeriod(10);
            }
        }
        int mCount = 0;
    };

    SwitchTask task;
    task.waitForEvent(1);
    REQUIRE(sched.addTask(task));

    // time passes before the event arrives
    tickInterrupt(sched, 5'000);

    sched.signalEvent(1);
    pumpPending();

    // it ran once, for the event - not a second time in the same pass
    CHECK(task.mCount == 1);

    tick_t nextDeadline = 0;
    REQUIRE(sched.tryGetNextDeadline(nextDeadline));
    CHECK(nextDeadline == tick_t(5'010));

    tickInterrupt(sched, 10);
    CHECK(task.mCount == 2);

    tickInterrupt(sched, 10);
    CHECK(task.mCount == 3);
}

TEST_CASE("TaskletScheduler - a nested scheduler can still be unscheduled") {

    // A scheduler is itself a task : removeTask(ITasklet&) must not hide the
    // no-argument removeTask() it inherits.

    resetHarness();
    TaskletScheduler<1> sched(periodic_backend);

    sched.removeTask();
    CHECK_FALSE(sched.isLinked());
}

TEST_CASE("TaskletScheduler - the backend is copied at construction") {

    // the scheduler doesn't depend on the TaskletBackend outliving it
    resetHarness();

    TaskletBackend hooks = oneshot_backend;
    TaskletScheduler<1> sched(hooks);
    hooks = {};

    g_now = 1'000;

    CountTask task;
    task.setPeriod(100);
    REQUIRE(sched.addTask(task));

    REQUIRE(g_wakeupArmed);
    CHECK(g_wakeupDeadline == tick_t(1'100));

    advanceClock(99);
    CHECK(task.mCount == 0);

    advanceClock(1);
    CHECK(task.mCount == 1);
    CHECK(g_suspendCount == 0);
}

TEST_CASE("TaskletScheduler - null optional hooks are replaced by no-ops") {

    // only getTick is provided : every other hook must be safe to call
    resetHarness();

    {
        TaskletScheduler<1> sched(TaskletBackend { getTick });

        CountTask eventTask;
        eventTask.waitForEvent(0);
        REQUIRE(sched.addTask(eventTask));
        sched.signalEvent(0);

        CountTask timerTask;
        timerTask.setPeriod(10);
        REQUIRE(sched.addTask(timerTask));
        g_now = 10;
        sched.poll();

        sched.removeTask(timerTask);
        sched.removeTask(eventTask);
    }

    // none of the harness hooks was reached
    CHECK_FALSE(g_pending);
    CHECK(g_handler == nullptr);
    CHECK_FALSE(g_wakeupArmed);
    CHECK(g_wakeupCalls == 0);
}

TEST_CASE("TaskletScheduler - thisTask() reports the task being run") {

    resetHarness();
    TaskletScheduler<1> sched(oneshot_backend);

    struct SelfTask : ITasklet {
        void run() override { mSeen = mScheduler->thisTask(); }
        TaskletScheduler<1>* mScheduler = nullptr;
        ITasklet* mSeen = nullptr;
    };

    SelfTask timerTask;
    timerTask.mScheduler = &sched;
    timerTask.setPeriod(10);
    REQUIRE(sched.addTask(timerTask));

    SelfTask eventTask;
    eventTask.mScheduler = &sched;
    eventTask.waitForEvent(0);
    REQUIRE(sched.addTask(eventTask));

    CHECK(sched.thisTask() == nullptr);

    advanceClock(10);
    CHECK(timerTask.mSeen == &timerTask);

    sched.signalEvent(0);
    pump();
    CHECK(eventTask.mSeen == &eventTask);

    // nothing is running once the scheduler returns
    CHECK(sched.thisTask() == nullptr);
}
