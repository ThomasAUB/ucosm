#include "tests.hpp"
#include "doctest.h"

#include "ucosm/tasklet/tasklet_scheduler.hpp"
#include "desktop_tasklet_executor.hpp"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace {

bool waitForExecutions(
    std::mutex& inMutex,
    std::condition_variable& inCondition,
    std::vector<int>& inExecuted,
    std::size_t inExpectedCount,
    std::chrono::milliseconds inTimeout = std::chrono::seconds(1)
) {
    std::unique_lock<std::mutex> lock(inMutex);
    return inCondition.wait_for(lock, inTimeout, [&] { return inExecuted.size() == inExpectedCount; });
}

// Blocks until the low priority worker is done with the handler it may be
// running, so that the scheduler state can be inspected - or a task removed -
// without racing the execution of run().
struct SchedulerBarrier final {
    SchedulerBarrier() { suspend_low_priority_execution(); }
    ~SchedulerBarrier() { resume_low_priority_execution(); }
    SchedulerBarrier(const SchedulerBarrier&) = delete;
    SchedulerBarrier& operator=(const SchedulerBarrier&) = delete;
};

// What a periodic tick interrupt does : move the platform's clock on, then
// let the scheduler pend its handler if a deadline has come round. The two
// are separate because the scheduler keeps no clock of its own - it reads the
// one the backend gives it, and poll() only makes it look.
template<ucosm::event_id_t event_count>
void tick(ucosm::TaskletScheduler<event_count>& inScheduler, ucosm::tick_t inInc = 1) {
    advance_tick(inInc);
    inScheduler.poll();
}

// Puts the clock back to its origin so each test case can reason in absolute
// deadlines. The scheduler under test is built after this, and takes its
// first deadlines from here.
struct ClockOrigin final {
    explicit ClockOrigin(ucosm::tick_t inValue = 0) { reset_tick(inValue); }
};

}

TEST_CASE("TaskletScheduler - timer wrap-around behavior") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct WrapTask : ucosm::ITasklet {
        WrapTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
            this->removeTask();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    WrapTask t1(1, &executed, &m, &cv);
    WrapTask t2(2, &executed, &m, &cv);

    // Move the scheduler close to wrap first, then schedule relative delays.
    tick(sched, static_cast<tick_t>(UINT32_MAX - 2));

    t1.setPeriod(10);
    t2.setPeriod(20);

    REQUIRE(sched.addTask(t1));
    REQUIRE(sched.addTask(t2));

    tick_t nextDeadline = 0;
    REQUIRE(sched.tryGetNextDeadline(nextDeadline));
    CHECK(nextDeadline == static_cast<tick_t>(7));

    tick(sched, 9);
    CHECK(executed.empty());

    tick(sched, 1);
    REQUIRE(waitForExecutions(m, cv, executed, 1));

    {
        SchedulerBarrier barrier;
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == static_cast<tick_t>(17));
    }

    tick(sched, 9);
    CHECK(executed.size() == 1);

    tick(sched, 1);
    REQUIRE(waitForExecutions(m, cv, executed, 2));

    REQUIRE(executed.size() == 2);
    CHECK(executed[0] == 1);
    CHECK(executed[1] == 2);

}

TEST_CASE("TaskletScheduler - concurrent signalEvent calls") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<4> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct EventTask : ucosm::ITasklet {
        EventTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
            this->removeTask();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    EventTask a(1, &executed, &m, &cv);
    EventTask b(2, &executed, &m, &cv);
    EventTask c(3, &executed, &m, &cv);

    a.setPriority(10);
    b.setPriority(5);
    c.setPriority(15);

    a.waitForEvent(0);
    b.waitForEvent(1);
    c.waitForEvent(2);

    REQUIRE(sched.addTask(a));
    REQUIRE(sched.addTask(b));
    REQUIRE(sched.addTask(c));

    // Suspend low-priority execution while we concurrently set pending events
    suspend_low_priority_execution();

    std::thread th1([&] { sched.signalEvent(0); });
    std::thread th2([&] { sched.signalEvent(1); });
    std::thread th3([&] { sched.signalEvent(2); });

    th1.join(); th2.join(); th3.join();

    resume_low_priority_execution();

    // wait for tasks to run
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, std::chrono::seconds(2), [&] { return executed.size() == 3; });
    }

    CHECK(executed.size() == 3);
    // expected order by priority: b (5), a (10), c (15)
    CHECK(executed[0] == 2);
    CHECK(executed[1] == 1);
    CHECK(executed[2] == 3);

}

TEST_CASE("TaskletScheduler - combined timer and event ordering") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<4> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct MixedTask : ucosm::ITasklet {
        MixedTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
            this->removeTask();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    MixedTask timerMid(1, &executed, &m, &cv); // mid priority but waits for the timer
    MixedTask eventHigh(2, &executed, &m, &cv);    // high priority, event
    MixedTask eventLow(3, &executed, &m, &cv);    // low priority, event

    timerMid.setPriority(10);
    eventHigh.setPriority(5); // high priority
    eventLow.setPriority(20);

    // timerMid will wake at tick 50
    timerMid.setPeriod(50);
    eventHigh.waitForEvent(0);
    eventLow.waitForEvent(1);

    REQUIRE(sched.addTask(timerMid));
    REQUIRE(sched.addTask(eventHigh));
    REQUIRE(sched.addTask(eventLow));

    // Queue both events and the timer wake-up before allowing the low-priority worker to run.
    suspend_low_priority_execution();
    tick(sched, 50);
    sched.signalEvent(0);
    sched.signalEvent(1);
    resume_low_priority_execution();

    REQUIRE(waitForExecutions(m, cv, executed, 3));

    CHECK(executed.size() == 3);
    CHECK(executed[0] == 2);
    CHECK(executed[1] == 1);
    CHECK(executed[2] == 3);

}

TEST_CASE("TaskletScheduler - event ordering") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<4> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct TestTask : ucosm::ITasklet {
        TestTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}

        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
            // remove ourselves so scheduler won't reschedule
            this->removeTask();
        }

        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    TestTask t1(1, &executed, &m, &cv);
    TestTask t2(2, &executed, &m, &cv);
    TestTask t3(3, &executed, &m, &cv);

    // priorities: lower numeric = higher scheduling precedence
    t1.setPriority(10);
    t2.setPriority(5);
    t3.setPriority(20);

    t1.waitForEvent(0);
    t2.waitForEvent(1);
    t3.waitForEvent(2);

    REQUIRE(sched.addTask(t1));
    REQUIRE(sched.addTask(t2));
    REQUIRE(sched.addTask(t3));

    // signal all events; run handler asynchronously
    sched.signalEvent(0);
    sched.signalEvent(1);
    sched.signalEvent(2);

    REQUIRE(waitForExecutions(m, cv, executed, 3, std::chrono::seconds(2)));

    CHECK(executed.size() == 3);

    // expected order by priority ascending: t2 (5), t1 (10), t3 (20)
    CHECK(executed[0] == 2);
    CHECK(executed[1] == 1);
    CHECK(executed[2] == 3);

}

TEST_CASE("TaskletScheduler - re-adding after reconfiguring updates its type") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct ReconfigurableTask : ucosm::ITasklet {
        ReconfigurableTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
            this->removeTask();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    SUBCASE("timer task reconfigured to wait for an event") {

        ReconfigurableTask t(1, &executed, &m, &cv);

        t.setPeriod(10);
        REQUIRE(sched.addTask(t));

        // Change its mind before the deadline: it should now run on the
        // event instead, not on tick().
        t.waitForEvent(0);
        REQUIRE(sched.addTask(t));

        tick(sched, 50);
        CHECK(executed.empty());

        sched.signalEvent(0);
        REQUIRE(waitForExecutions(m, cv, executed, 1));
        CHECK(executed[0] == 1);
    }

    SUBCASE("event task reconfigured to wait for the timer") {

        ReconfigurableTask t(2, &executed, &m, &cv);

        t.waitForEvent(0);
        REQUIRE(sched.addTask(t));

        // Change its mind: it should now run on the timer instead, not on
        // the event it was previously waiting for.
        t.setPeriod(10);
        REQUIRE(sched.addTask(t));

        sched.signalEvent(0);
        CHECK(executed.empty());

        tick(sched, 10);
        REQUIRE(waitForExecutions(m, cv, executed, 1));
        CHECK(executed[0] == 2);
    }

}

TEST_CASE("TaskletScheduler - unconfigured task is rejected") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    struct NeverConfiguredTask : ucosm::ITasklet {
        void run() override {}
    };

    NeverConfiguredTask t;

    // Never went through setPeriod()/waitForEvent(): nothing to
    // schedule, so addTask() must refuse rather than silently guessing.
    CHECK_FALSE(sched.addTask(t));
    CHECK_FALSE(sched.addTask(t, 10));
    CHECK_FALSE(t.isLinked());
}

TEST_CASE("TaskletScheduler - timer wake ordering") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct TimerTask : ucosm::ITasklet {
        TimerTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}

        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
            this->removeTask();
        }

        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    TimerTask s1(1, &executed, &m, &cv);
    TimerTask s2(2, &executed, &m, &cv);
    TimerTask s3(3, &executed, &m, &cv);

    // set periods so they wake in order 2,1,3
    s1.setPeriod(50);
    s2.setPeriod(10);
    s3.setPeriod(100);

    // add tasks (they will be inserted into the timer list)
    REQUIRE(sched.addTask(s1));
    REQUIRE(sched.addTask(s2));
    REQUIRE(sched.addTask(s3));

    tick_t nextDeadline = 0;
    REQUIRE(sched.tryGetNextDeadline(nextDeadline));
    CHECK(nextDeadline == 10);

    tick(sched, 10);
    REQUIRE(waitForExecutions(m, cv, executed, 1));

    CHECK(executed.size() == 1);
    CHECK(executed[0] == 2);

    {
        SchedulerBarrier barrier;
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 50);
    }

    // advance from tick 10 to tick 50 -> should wake s1
    tick(sched, 40);
    REQUIRE(waitForExecutions(m, cv, executed, 2));

    CHECK(executed.size() == 2);
    CHECK(executed[1] == 1);

    {
        SchedulerBarrier barrier;
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 100);
    }

    // advance from tick 50 to tick 100 -> should wake s3
    tick(sched, 50);
    REQUIRE(waitForExecutions(m, cv, executed, 3));

    CHECK(executed.size() == 3);
    CHECK(executed[2] == 3);

}

TEST_CASE("TaskletScheduler - the period is kept across runs") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    // Never reconfigures itself : the scheduler re-arms it with the same
    // period, so setPeriod() alone makes the task periodic.
    struct PeriodicTask : ucosm::ITasklet {
        PeriodicTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    PeriodicTask t(1, &executed, &m, &cv);

    t.setPeriod(10);
    REQUIRE(sched.addTask(t));

    tick_t nextDeadline = 0;
    REQUIRE(sched.tryGetNextDeadline(nextDeadline));
    CHECK(nextDeadline == 10);

    tick(sched, 10);
    REQUIRE(waitForExecutions(m, cv, executed, 1));

    {
        SchedulerBarrier barrier;
        CHECK(t.isLinked());
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 20);
    }

    tick(sched, 10);
    REQUIRE(waitForExecutions(m, cv, executed, 2));

    {
        SchedulerBarrier barrier;
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 30);
    }

    // A late wake-up (dispatched at 35 for a deadline due at 30) does not
    // shift the period : the next deadline stays on the original grid (40),
    // counted from the deadline that was due rather than from the moment the
    // task happened to be dispatched. Only a task that comes back out of
    // run() already overdue against that next deadline falls back to being
    // counted from there, so a genuinely stale wake-up still doesn't trap the
    // handler catching up.
    tick(sched, 15);
    REQUIRE(waitForExecutions(m, cv, executed, 3));

    {
        SchedulerBarrier barrier;
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 40);
    }

    CHECK(executed[0] == 1);
    CHECK(executed[1] == 1);
    CHECK(executed[2] == 1);

    {
        SchedulerBarrier barrier;
        sched.removeTask(t);
    }

    CHECK_FALSE(t.isLinked());

    tick(sched, 100);

    CHECK_FALSE(waitForExecutions(m, cv, executed, 4, std::chrono::milliseconds(100)));

}

TEST_CASE("TaskletScheduler - a task disposing of itself is dropped") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    // Ends its own scheduling without removeTask() : the scheduler has no list
    // to push it into, and must unlink it rather than keep it in the run list.
    struct DisposingTask : ucosm::ITasklet {
        DisposingTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
            this->dispose();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    DisposingTask t(1, &executed, &m, &cv);

    t.setPeriod(10);
    REQUIRE(sched.addTask(t));

    tick(sched, 10);
    REQUIRE(waitForExecutions(m, cv, executed, 1));

    {
        SchedulerBarrier barrier;
        CHECK_FALSE(t.isLinked());
        CHECK_FALSE(t.isConfigured());
    }

    tick(sched, 100);

    CHECK_FALSE(waitForExecutions(m, cv, executed, 2, std::chrono::milliseconds(100)));

    // and it stays unschedulable until it is configured again
    CHECK_FALSE(sched.addTask(t));

    t.setPeriod(10);
    REQUIRE(sched.addTask(t));

    {
        SchedulerBarrier barrier;
        sched.removeTask(t);
    }

}

TEST_CASE("TaskletScheduler - event subscription is kept across runs") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    // Symmetrically with setPeriod(), a task that leaves its state untouched
    // stays subscribed to the event it was waiting for.
    struct SubscribedTask : ucosm::ITasklet {
        SubscribedTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    SubscribedTask t(1, &executed, &m, &cv);

    t.waitForEvent(0);
    REQUIRE(sched.addTask(t));

    sched.signalEvent(0);
    REQUIRE(waitForExecutions(m, cv, executed, 1));

    sched.signalEvent(0);
    REQUIRE(waitForExecutions(m, cv, executed, 2));

    {
        SchedulerBarrier barrier;
        CHECK(t.isLinked());
        sched.removeTask(t);
    }

    sched.signalEvent(0);

    CHECK_FALSE(waitForExecutions(m, cv, executed, 3, std::chrono::milliseconds(100)));

}

TEST_CASE("TaskletScheduler - delay applies once, then the period takes over") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct DelayedTask : ucosm::ITasklet {
        DelayedTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    DelayedTask t(1, &executed, &m, &cv);

    // first execution after 10 ticks, then every 100
    t.setPeriod(100);
    REQUIRE(sched.addTask(t, 10));

    tick_t nextDeadline = 0;
    REQUIRE(sched.tryGetNextDeadline(nextDeadline));
    CHECK(nextDeadline == 10);

    tick(sched, 10);
    REQUIRE(waitForExecutions(m, cv, executed, 1));

    {
        SchedulerBarrier barrier;
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 110);
    }

    tick(sched, 100);
    REQUIRE(waitForExecutions(m, cv, executed, 2));

    {
        SchedulerBarrier barrier;
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 210);
        sched.removeTask(t);
    }

}

TEST_CASE("TaskletScheduler - a task can delay its next execution from run()") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    // Asks for a shorter delay on its first run only : the period must be
    // back in place for the executions after that one.
    struct SelfDelayingTask : ucosm::ITasklet {
        SelfDelayingTask(
            int id,
            ucosm::TaskletScheduler<2>* sched,
            std::vector<int>* out,
            std::mutex* m,
            std::condition_variable* cv
        ) :
            mID(id), mSched(sched), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            bool firstRun = false;
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
                firstRun = (mOut->size() == 1);
            }
            if (firstRun) {
                // checked from the main thread : doctest assertions don't
                // belong in the low priority handler
                mDelayed = mSched->setDelay(*this, 5);
            }
            mCV->notify_one();
        }
        int mID;
        bool mDelayed = false;
        ucosm::TaskletScheduler<2>* mSched;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    SelfDelayingTask t(1, &sched, &executed, &m, &cv);

    t.setPeriod(20);
    REQUIRE(sched.addTask(t));

    tick(sched, 20);
    REQUIRE(waitForExecutions(m, cv, executed, 1));

    tick_t nextDeadline = 0;

    {
        SchedulerBarrier barrier;
        CHECK(t.mDelayed);
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 25);
    }

    tick(sched, 5);
    REQUIRE(waitForExecutions(m, cv, executed, 2));

    {
        SchedulerBarrier barrier;
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 45);
        sched.removeTask(t);
    }

}

TEST_CASE("TaskletScheduler - setDelay re-arms a scheduled task") {

    using namespace ucosm;

    ClockOrigin origin;
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct WaitingTask : ucosm::ITasklet {
        WaitingTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
            mID(id), mOut(out), mM(m), mCV(cv) {}
        void run() override {
            {
                std::lock_guard<std::mutex> lk(*mM);
                mOut->push_back(mID);
            }
            mCV->notify_one();
        }
        int mID;
        std::vector<int>* mOut;
        std::mutex* mM;
        std::condition_variable* mCV;
    };

    tick_t nextDeadline = 0;

    SUBCASE("on a timer task") {

        WaitingTask t(1, &executed, &m, &cv);

        t.setPeriod(100);
        REQUIRE(sched.addTask(t));

        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 100);

        // bring the next execution forward without changing the period
        REQUIRE(sched.setDelay(t, 10));

        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 10);

        tick(sched, 10);
        REQUIRE(waitForExecutions(m, cv, executed, 1));

        {
            SchedulerBarrier barrier;
            REQUIRE(sched.tryGetNextDeadline(nextDeadline));
            CHECK(nextDeadline == 110);
            sched.removeTask(t);
        }
    }

    SUBCASE("on a task waiting for an event") {

        WaitingTask t(2, &executed, &m, &cv);

        t.waitForEvent(0);
        REQUIRE(sched.addTask(t));

        // a delay is meaningless for an event : the task is left alone,
        // switching it to the timer is setPeriod()'s job
        CHECK_FALSE(sched.setDelay(t, 10));
        CHECK_FALSE(sched.tryGetNextDeadline(nextDeadline));

        sched.signalEvent(0);
        REQUIRE(waitForExecutions(m, cv, executed, 1));

        {
            SchedulerBarrier barrier;
            sched.removeTask(t);
        }
    }

    SUBCASE("on a task that isn't scheduled yet") {

        WaitingTask t(3, &executed, &m, &cv);

        t.setPeriod(100);

        // the delay lives in the task rank, which only means something once
        // the task is in the timer list
        CHECK_FALSE(sched.setDelay(t, 10));
        CHECK_FALSE(sched.tryGetNextDeadline(nextDeadline));

        REQUIRE(sched.addTask(t, 10));
        REQUIRE(sched.tryGetNextDeadline(nextDeadline));
        CHECK(nextDeadline == 10);

        {
            SchedulerBarrier barrier;
            sched.removeTask(t);
        }
    }

}
