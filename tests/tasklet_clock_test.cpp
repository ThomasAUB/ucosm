#include "tests.hpp"
#include "doctest.h"

#include "ucosm/tasklet/tasklet_scheduler.hpp"

// Tests for TaskletBackend::readTick, the mode where the platform owns the
// clock and wakes the scheduler on the deadlines it is given, rather than
// advancing a software clock with tick().
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

    tick_t readTick() { return g_now; }

    const TaskletBackend clock_backend {
        requestExecution,
        installHandler,
        suspendExecution,
        resumeExecution,
        scheduleNextWakeup,
        readTick
    };

    // Same platform, minus the clock : time is then whatever tick() counted.
    const TaskletBackend tick_backend {
        requestExecution,
        installHandler,
        suspendExecution,
        resumeExecution,
        scheduleNextWakeup,
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
    // alone would. For a scheduler on the software clock, where the platform's
    // clock below means nothing and only tick() makes a task due.
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
    // made due.
    void advanceClock(tick_t inDelta) {
        g_now += inDelta;
        pump();
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

TEST_CASE("TaskletScheduler - backend clock : a task armed after an idle stretch is not due early") {

    resetHarness();
    TaskletScheduler<1> sched(clock_backend);

    // Time passes with nothing scheduled, so nothing wakes the scheduler and
    // nothing refreshes its idea of the time. This is the case a software
    // clock cannot get right : it would still believe it is at 0.
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

TEST_CASE("TaskletScheduler - software clock : time is what tick() counted") {

    // The contrast with the test above, and the reason readTick exists. With
    // no clock from the backend, time is defined by tick() : a platform that
    // has not called it has, as far as the scheduler is concerned, not let any
    // time pass, and the deadline comes out relative to that.

    resetHarness();
    TaskletScheduler<1> sched(tick_backend);

    g_now = 5'000; // the platform's clock, which this scheduler cannot read

    CountTask task;
    task.setPeriod(500);
    REQUIRE(sched.addTask(task));

    REQUIRE(g_wakeupArmed);
    CHECK(g_wakeupDeadline == tick_t(500));

    // and it is tick(), not the platform's clock, that makes it due
    sched.tick(499);
    pumpPending();
    CHECK(task.mCount == 0);

    sched.tick(1);
    pumpPending();
    CHECK(task.mCount == 1);
}

TEST_CASE("TaskletScheduler - backend clock : tick() no longer moves time") {

    resetHarness();
    TaskletScheduler<1> sched(clock_backend);

    g_now = 100;

    CountTask task;
    task.setPeriod(50);
    REQUIRE(sched.addTask(task));
    CHECK(g_wakeupDeadline == tick_t(150));

    // ignored : the software clock it would advance is unused
    sched.tick(1'000);
    pump();
    CHECK(task.mCount == 0);
    CHECK(g_wakeupDeadline == tick_t(150));

    advanceClock(50);
    CHECK(task.mCount == 1);
}

TEST_CASE("TaskletScheduler - backend clock : a period is re-armed from the clock") {

    resetHarness();
    TaskletScheduler<1> sched(clock_backend);

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

TEST_CASE("TaskletScheduler - backend clock : the deadline is the earliest of the tasks") {

    resetHarness();
    TaskletScheduler<1> sched(clock_backend);

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

TEST_CASE("TaskletScheduler - backend clock : a task slower than its period does not trap run()") {

    resetHarness();
    TaskletScheduler<1> sched(clock_backend);

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

TEST_CASE("TaskletScheduler - backend clock : a task faster than its period keeps an exact period") {

    resetHarness();
    TaskletScheduler<1> sched(clock_backend);

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

TEST_CASE("TaskletScheduler - backend clock : an interrupt task runs without any deadline") {

    resetHarness();
    TaskletScheduler<2> sched(clock_backend);

    CountTask task;
    task.waitForInterrupt(1);
    REQUIRE(sched.addTask(task));

    // nothing sleeping, so the platform's timer stays off
    CHECK_FALSE(g_wakeupArmed);

    g_now = 9'999;

    sched.signalInterrupt(1);
    pump();
    CHECK(task.mCount == 1);

    sched.signalInterrupt(1);
    pump();
    CHECK(task.mCount == 2);

    CHECK_FALSE(g_wakeupArmed);
}

TEST_CASE("TaskletScheduler - backend clock : a task armed across the tick wrap is not skipped") {

    // Deadlines are cyclic, so around the moment the clock wraps past zero the
    // timer list holds both kinds at once : deadlines that have wrapped,
    // numerically tiny, and the cursor they are ahead of, numerically huge.
    // Ordering them on the raw value rather than on the delay separating them
    // from the cursor files a wrapped deadline in front of it, which is the one
    // place the scheduler never looks for the next task to run.

    resetHarness();
    TaskletScheduler<1> sched(clock_backend);

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

TEST_CASE("TaskletScheduler - backend clock : removeTask drops the deadline it was armed on") {

    resetHarness();
    TaskletScheduler<1> sched(clock_backend);

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
