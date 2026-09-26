#include <chrono>
#include <cstdint>

#include "tests.hpp"
#include "doctest.h"

#include "ucosm/tasklet/tasklet_scheduler.hpp"

// Randomized stress and throughput benchmark of the TaskletScheduler lists.
//
// Like tasklet_clock_test.cpp, the platform is modelled synchronously : a
// flag stands for PendSV's pending bit and pump() plays the handler, so time
// only moves when the test moves it and every invariant can be checked
// between two steps without racing anything.

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
    void noop() {}

    const TaskletBackend stress_backend {
        getTick,
        requestExecution,
        installHandler,
        noop,
        noop,
        nullptr
    };

    void resetHarness(tick_t inNow) {
        g_now = inNow;
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

    // xorshift32 : deterministic, so that a failure replays identically
    struct Rng {
        uint32_t mState;
        uint32_t operator()() {
            mState ^= mState << 13;
            mState ^= mState >> 17;
            mState ^= mState << 5;
            return mState;
        }
    };

    constexpr event_id_t stress_events = 3;
    constexpr tick_t max_delay = 60;

    // Exposes the scheduler lists so the test can check their invariants.
    struct InspectedScheduler : TaskletScheduler<stress_events> {

        using TaskletScheduler<stress_events>::TaskletScheduler;

        // Returns the number of tasks found in all lists, or -1 when an
        // invariant is broken.
        int checkLists() {

            int count = 0;

            // timer list : nothing before the cursor, then sorted by the
            // delay separating each deadline from the cursor
            const auto cursor = this->mCursorTask.getRank();
            bool cursorSeen = false;
            tick_t previousDelay = 0;

            for (auto& t : this->mTimerList) {
                if (&t == &this->mCursorTask) {
                    cursorSeen = true;
                    continue;
                }
                if (!cursorSeen) {
                    return -1;
                }
                const auto delay = getDeadlineDelay(cursor, t.getRank());
                if (delay < previousDelay) {
                    return -1;
                }
                previousDelay = delay;
                ++count;
            }

            if (!cursorSeen) {
                return -1;
            }

            // event lists : sorted by priority, which is also the rank
            for (auto& list : this->mEventTaskLists) {
                bool first = true;
                priority_t previous = 0;
                for (auto& t : list) {
                    const auto& tasklet = static_cast<const ITasklet&>(t);
                    if (t.getRank() != tasklet.getPriority()) {
                        return -1;
                    }
                    if (!first && t.getRank() < previous) {
                        return -1;
                    }
                    previous = t.getRank();
                    first = false;
                    ++count;
                }
            }

            return count;
        }

        // Every timer task must be strictly in the future once the
        // scheduler settled, and no further away than the longest delay the
        // test ever asks for.
        bool noTimerOverdue() {
            for (auto& t : this->mTimerList) {
                if (&t == &this->mCursorTask) {
                    continue;
                }
                const auto delay = getDeadlineDelay(g_now, t.getRank());
                if (delay == 0 || delay > max_delay) {
                    return false;
                }
            }
            return true;
        }
    };

    InspectedScheduler* g_sched = nullptr;

    // A task that randomly reconfigures itself from within run(), covering
    // every path the scheduler takes after a task returns.
    struct ChaosTask : ITasklet {

        void run() override {
            ++mRuns;
            switch (mRng() % 8) {
                case 0: setPeriod(1 + mRng() % 40); break;
                case 1: waitForEvent(static_cast<event_id_t>(mRng() % stress_events)); break;
                case 2: this->removeTask(); break;
                case 3: g_sched->setDelay(*this, mRng() % max_delay); break;
                case 4: dispose(); break;
                default: break; // keep the current configuration
            }
        }

        Rng mRng { 1 };
        int mRuns = 0;
    };

    void configureRandomly(ChaosTask& ioTask, Rng& ioRng) {
        ioTask.setPriority(ioRng() % 8);
        if (ioRng() % 3 == 0) {
            ioTask.waitForEvent(static_cast<event_id_t>(ioRng() % stress_events));
        }
        else {
            ioTask.setPeriod(1 + ioRng() % 40);
        }
    }

}

TEST_CASE("TaskletScheduler - stress : lists stay consistent under random operations") {

    constexpr int rounds = 50;
    constexpr int steps = 4000;
    constexpr int task_count = 24;

    Rng rng { 0x1234567u };
    int totalRuns = 0;

    for (int round = 0; round < rounds; ++round) {

        // starts close to the counter wrap, which every round goes through
        resetHarness(static_cast<tick_t>(0u - 2'000u + rng() % 1'000u));

        InspectedScheduler sched(stress_backend);
        g_sched = &sched;

        ChaosTask tasks[task_count];
        for (int i = 0; i < task_count; ++i) {
            tasks[i].mRng.mState = rng() | 1u;
        }

        for (int step = 0; step < steps; ++step) {

            auto& task = tasks[rng() % task_count];
            bool settled = false;

            switch (rng() % 8) {
                case 0:
                    if (!task.isLinked()) {
                        configureRandomly(task, rng);
                        sched.addTask(task);
                    }
                    break;
                case 1:
                    if (!task.isLinked()) {
                        task.setPriority(rng() % 8);
                        task.setPeriod(1 + rng() % 40);
                        sched.addTask(task, rng() % max_delay);
                    }
                    break;
                case 2:
                    sched.setDelay(task, rng() % max_delay);
                    break;
                case 3:
                    sched.removeTask(task);
                    break;
                case 4:
                    sched.signalEvent(static_cast<event_id_t>(rng() % stress_events));
                    pump();
                    break;
                default:
                    g_now += rng() % 4;
                    sched.poll();
                    pump();
                    settled = true;
                    break;
            }

            int linked = 0;
            for (auto& t : tasks) {
                linked += t.isLinked() ? 1 : 0;
            }

            const int listed = sched.checkLists();

            INFO("round ", round, " step ", step);
            REQUIRE(listed >= 0);
            REQUIRE(listed == linked);

            if (settled) {
                REQUIRE(sched.noTimerOverdue());
            }
        }

        for (auto& t : tasks) {
            totalRuns += t.mRuns;
        }

        g_sched = nullptr;
    }

    // the random walk must actually have exercised dispatch
    CHECK(totalRuns > rounds * steps / 10);
}

TEST_CASE("TaskletScheduler - benchmark : dispatch throughput") {

    struct CountTask : ITasklet {
        void run() override { ++mCount; }
        uint32_t mCount = 0;
    };

    using clock = std::chrono::steady_clock;

    constexpr int task_count = 64;
    constexpr event_id_t event_count = 8;
    constexpr uint32_t ticks = 50'000;

    // Mixed periods spread the deadlines over the timer list, a single
    // period makes every task due at once on every tick.
    for (const bool mixed : { true, false }) {

        resetHarness(0);

        TaskletScheduler<event_count> sched(stress_backend);

        CountTask timerTasks[task_count];
        CountTask eventTasks[event_count];

        for (int i = 0; i < task_count; ++i) {
            timerTasks[i].setPriority(i % 7);
            timerTasks[i].setPeriod(mixed ? 1 + (i * 37) % 50 : 1);
            REQUIRE(sched.addTask(timerTasks[i]));
        }

        for (event_id_t i = 0; i < event_count; ++i) {
            eventTasks[i].setPriority(i);
            eventTasks[i].waitForEvent(i);
            REQUIRE(sched.addTask(eventTasks[i]));
        }

        const auto start = clock::now();

        for (uint32_t k = 0; k < ticks; ++k) {
            ++g_now;
            sched.poll();
            sched.signalEvent(static_cast<event_id_t>(k % event_count));
            pump();
        }

        const auto elapsed = clock::now() - start;

        uint64_t executions = 0;

        // each task ran exactly once per period, none was skipped or doubled
        for (auto& t : timerTasks) {
            CHECK(t.mCount == ticks / t.getPeriod());
            executions += t.mCount;
        }

        for (auto& t : eventTasks) {
            CHECK(t.mCount == ticks / event_count);
            executions += t.mCount;
        }

        const auto ns = static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());

        MESSAGE(
            (mixed ? "mixed periods" : "single period"),
            " : ", ns / ticks, " ns/tick, ",
            ns / static_cast<double>(executions), " ns/execution"
        );
    }
}
