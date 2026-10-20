#include "tests.hpp"
#include "doctest.h"

#include "ucosm/tasklet/tasklet_scheduler.hpp"
#include <vector>

// Tests for TaskletEvent : an event is an object owned by the code that
// signals it, so that independent modules never share an identifier space.
//
// Everything here is synchronous : the deferred execution is a flag, run by
// pump(), so that a test can signal and inspect without racing anything.

namespace {

    using namespace ucosm;

    tick_t g_now = 0;
    bool g_pending = false;
    void (*g_handler)(void*) = nullptr;
    void* g_context = nullptr;

    tick_t getTick() { return g_now; }

    void requestExecution() { g_pending = true; }

    void installHandler(void (*inHandler)(void*), void* inContext) {
        g_handler = inHandler;
        g_context = inContext;
    }

    const TaskletBackend event_backend {
        getTick,
        requestExecution,
        installHandler,
        nullptr,
        nullptr,
        nullptr
    };

    // a scheduler that is never dispatched, to check the binding rules
    const TaskletBackend idle_backend { getTick };

    void resetHarness() {
        g_now = 0;
        g_pending = false;
        g_handler = nullptr;
        g_context = nullptr;
    }

    void pump() {
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

    struct RecordTask : ITasklet {
        RecordTask(int inID, std::vector<int>& outLog) : mID(inID), mLog(outLog) {}
        void run() override { mLog.push_back(mID); }
        int mID;
        std::vector<int>& mLog;
    };

}

TEST_CASE("TaskletEvent - waiting through a const reference") {

    resetHarness();
    TaskletScheduler sched(event_backend);
    TaskletEvent event;

    std::vector<int> log;
    RecordTask t(1, log);

    // what an owner hands out : a wait-only view of its event
    const TaskletEvent& view = event;
    t.waitForEvent(view);
    CHECK(t.getEvent() == &event);
    REQUIRE(sched.addTask(t));

    event.signal();
    pump();
    CHECK(log == std::vector<int> { 1 });
}

TEST_CASE("TaskletEvent - a signal sent before any wait is dropped") {

    resetHarness();
    TaskletScheduler sched(event_backend);
    TaskletEvent event;

    std::vector<int> log;
    RecordTask t(1, log);

    event.signal();
    CHECK_FALSE(g_pending);

    t.waitForEvent(event);
    REQUIRE(sched.addTask(t));
    pump();
    CHECK(log.empty());

    event.signal();
    pump();
    CHECK(log == std::vector<int> { 1 });
}

TEST_CASE("TaskletEvent - independent events don't interfere") {

    resetHarness();
    TaskletScheduler sched(event_backend);

    // as two libraries would each declare their own
    TaskletEvent eventA;
    TaskletEvent eventB;

    std::vector<int> log;
    RecordTask a(1, log);
    RecordTask b(2, log);

    a.waitForEvent(eventA);
    b.waitForEvent(eventB);
    REQUIRE(sched.addTask(a));
    REQUIRE(sched.addTask(b));

    eventB.signal();
    pump();
    CHECK(log == std::vector<int> { 2 });

    eventA.signal();
    pump();
    CHECK(log == std::vector<int> { 2, 1 });
}

TEST_CASE("TaskletEvent - waiters of signaled events run by priority") {

    resetHarness();
    TaskletScheduler sched(event_backend);
    TaskletEvent eventA;
    TaskletEvent eventB;

    std::vector<int> log;
    RecordTask low(3, log);
    RecordTask high(1, log);
    RecordTask mid(2, log);

    low.setPriority(30);
    high.setPriority(10);
    mid.setPriority(20);

    low.waitForEvent(eventA);
    high.waitForEvent(eventB);
    mid.waitForEvent(eventA);

    REQUIRE(sched.addTask(low));
    REQUIRE(sched.addTask(high));
    REQUIRE(sched.addTask(mid));

    eventA.signal();
    eventB.signal();
    pump();
    CHECK(log == std::vector<int> { 1, 2, 3 });

    // coalesced : several signals before the dispatch run the waiters once
    log.clear();
    eventA.signal();
    eventA.signal();
    pump();
    CHECK(log == std::vector<int> { 2, 3 });
}

TEST_CASE("TaskletEvent - a signal sent while its waiter runs runs it again") {

    resetHarness();
    TaskletScheduler sched(event_backend);
    TaskletEvent event;

    // stands for an interrupt signaling while the task drains its input
    struct ResignalTask : ITasklet {
        void run() override {
            if (++mRuns == 1) {
                mEvent->signal();
            }
        }
        TaskletEvent* mEvent = nullptr;
        int mRuns = 0;
    };

    ResignalTask t;
    t.mEvent = &event;
    t.waitForEvent(event);
    REQUIRE(sched.addTask(t));

    event.signal();
    pump();
    CHECK(t.mRuns == 2);
}

TEST_CASE("TaskletEvent - destroying an event disposes its waiters") {

    resetHarness();
    TaskletScheduler sched(event_backend);

    std::vector<int> log;
    RecordTask t(1, log);

    {
        TaskletEvent event;
        t.waitForEvent(event);
        REQUIRE(sched.addTask(t));
        CHECK(t.isLinked());
    }

    CHECK_FALSE(t.isLinked());
    CHECK_FALSE(t.isConfigured());
    CHECK(t.getEvent() == nullptr);
    CHECK_FALSE(sched.addTask(t));
}

TEST_CASE("TaskletEvent - an event binds to the scheduler of its waiters") {

    resetHarness();
    TaskletScheduler sched(event_backend);
    TaskletScheduler other(idle_backend);
    TaskletEvent event;

    std::vector<int> log;
    RecordTask a(1, log);
    RecordTask b(2, log);

    a.waitForEvent(event);
    b.waitForEvent(event);
    REQUIRE(sched.addTask(a));
    CHECK_FALSE(other.addTask(b));

    // released lazily : the next dispatch finds it without waiters
    sched.removeTask(a);
    event.signal();
    pump();
    CHECK(log.empty());

    CHECK(other.addTask(b));
}

TEST_CASE("TaskletEvent - a destroyed scheduler releases its events") {

    resetHarness();
    TaskletEvent event;

    std::vector<int> log;
    RecordTask t(1, log);

    {
        TaskletScheduler sched(event_backend);
        t.waitForEvent(event);
        REQUIRE(sched.addTask(t));
    }

    CHECK_FALSE(t.isLinked());

    // unbound : nothing to wake
    event.signal();
    CHECK_FALSE(g_pending);

    TaskletScheduler sched(event_backend);
    REQUIRE(sched.addTask(t));

    event.signal();
    pump();
    CHECK(log == std::vector<int> { 1 });
}

TEST_CASE("TaskletEvent - a waiter switching to a timer leaves the event") {

    resetHarness();
    TaskletScheduler sched(event_backend);
    TaskletEvent event;

    struct SwitchTask : ITasklet {
        void run() override {
            ++mRuns;
            setPeriod(10);
        }
        int mRuns = 0;
    };

    SwitchTask t;
    t.waitForEvent(event);
    REQUIRE(sched.addTask(t));

    event.signal();
    pump();
    CHECK(t.mRuns == 1);
    CHECK(t.isWaitingForTimer());

    // no longer a waiter : the signal doesn't run it
    event.signal();
    pump();
    CHECK(t.mRuns == 1);

    g_now = 10;
    sched.poll();
    pump();
    CHECK(t.mRuns == 2);
}
