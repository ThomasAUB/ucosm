#include "tests.hpp"
#include "doctest.h"

#include "ucosm/resumable/iresumable_task.hpp"
#include "ucosm/periodic/periodic_scheduler.hpp"

#include <vector>
#include <iostream>

TEST_CASE("Coroutine task test") {

    StreamSilencer silence(std::cout);

    struct StateMachineTask : ucosm::IResumableTask {

        bool connectionSuccessful() {
            static int sSuccess = 0;
            return sSuccess++ >= 3;
        }

        bool responseReceived() {
            static int sRX = 0;
            return sRX++ >= 10;
        }

        enum State { IDLE, CONNECTING, SENDING, WAITING, DONE };
        int mAttempts = 0;
        int mRetries = 0;

        std::vector<State> mStateList;

        void run() override {

            UCOSM_START;

            mStateList.push_back(CONNECTING);
            std::cout << "Connecting..." << std::endl;

            UCOSM_SLEEP_FOR(500);  // Connection delay

            if (connectionSuccessful()) {
                mStateList.push_back(SENDING);
                std::cout << "Sending data..." << std::endl;
            }
            else {
                std::cout << "Connection failed, retrying..." << std::endl;
                UCOSM_RESTART;  // Restart from beginning
            }

            UCOSM_SLEEP_FOR(200);  // Send delay

            mStateList.push_back(WAITING);
            std::cout << "Waiting for response..." << std::endl;

            mRetries = 0;
            mAttempts = 0;

            UCOSM_SLEEP_UNTIL(responseReceived() || mRetries++ == 3, 100); // timeout at 300ms

            if (responseReceived()) {
                std::cout << "Success!" << std::endl;
                mStateList.push_back(DONE);
            }
            else if (++mAttempts < 3) {
                std::cout << "Timeout, retrying..." << std::endl;
                mStateList.push_back(SENDING);
                UCOSM_RESTART;
            }
            else {
                std::cout << "Max retries reached" << std::endl;
            }

            UCOSM_END;
        }

    };


    ucosm::PeriodicScheduler sched(getMillis);

    StateMachineTask t;

    sched.addTask(t);

    std::cout << "\n=== Resumable Task start ===\n" << std::endl;

    while (!sched.empty()) {
        sched.run();
    }

    static constexpr StateMachineTask::State checkStates[] = {

        StateMachineTask::State::CONNECTING,
        StateMachineTask::State::CONNECTING,
        StateMachineTask::State::CONNECTING,
        StateMachineTask::State::CONNECTING,

        StateMachineTask::State::SENDING,
        StateMachineTask::State::WAITING,

        StateMachineTask::State::SENDING,
        StateMachineTask::State::CONNECTING,

        StateMachineTask::State::SENDING,
        StateMachineTask::State::WAITING,

        StateMachineTask::State::SENDING,
        StateMachineTask::State::CONNECTING,

        StateMachineTask::State::SENDING,
        StateMachineTask::State::WAITING,

        StateMachineTask::State::DONE
    };

    CHECK(t.mStateList.size() == sizeof(checkStates) / sizeof(checkStates[0]));

    int i = 0;
    for (auto s : t.mStateList) {
        CHECK(s == checkStates[i++]);
    }

    std::cout << "\n=== Resumable Task end ===\n" << std::endl;
}

TEST_CASE("UCOSM_SLEEP_UNTIL pacing") {

    StreamSilencer silence(std::cout);

    static uint32_t sNow = 0;
    static bool sFlag = false;

    // Verifies that SLEEP_UNTIL re-arms the period between checks
    // instead of busy-polling every tick. The task records every time
    // its body starts; with check_period=100 and sFlag flipping at 350,
    // we expect the body to execute at 0, 100, 200, 300, then complete at 400.
    struct WaitTask : ucosm::IResumableTask {

        uint32_t mCheckPeriod = 100;
        std::vector<uint32_t> mBodyTicks;
        bool mDone = false;

        void run() override {

            UCOSM_START;

            // Record each fresh entry into the body (before the first
            // SLEEP_UNTIL). After yields we resume inside SLEEP_UNTIL,
            // so this line runs once per (re)start of the task.
            mBodyTicks.push_back(sNow);

            UCOSM_SLEEP_UNTIL(sFlag, mCheckPeriod);

            mDone = true;
            UCOSM_END;
        }
    };

    ucosm::PeriodicScheduler sched(
        +[] () { return sNow; }
    );

    WaitTask t;
    t.setPeriod(0);
    sched.addTask(t);

    // drive time manually. The scheduler updates the task rank to
    // now + period after each run; we observe the resulting period
    // to confirm SLEEP_UNTIL re-arms instead of busy-polling.
    sNow = 0;
    sched.run();
    CHECK(t.getPeriod() == 100); // re-armed with check_period

    sNow = 100;
    sched.run();
    CHECK(t.getPeriod() == 100); // still re-arming, not 0

    sNow = 200;
    sched.run();
    CHECK(t.getPeriod() == 100);

    sNow = 300;
    sched.run();
    CHECK(t.getPeriod() == 100);

    sFlag = true;
    sNow = 400;
    sched.run();
    CHECK(t.mDone);
    // task completed and removed itself
    CHECK(sched.empty());

    // With the old busy-poll bug, period would be 0 after the first
    // recheck, causing the scheduler to run the task every tick. Here
    // we confirm the task only ran when its deadline (now+100) was due.
    CHECK(t.mBodyTicks.size() == 1);
    CHECK(t.mBodyTicks[0] == 0);
}