#include "tests.hpp"
#include "doctest.h"

#include "ucosm/resumable/iresumable_task.hpp"
#include "ucosm/periodic/periodic_scheduler.hpp"

#include <vector>
#include <iostream>

TEST_CASE("Coroutine task test") {

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

            UCOSM_WAIT(500);  // Connection delay

            if (connectionSuccessful()) {
                mStateList.push_back(SENDING);
                std::cout << "Sending data..." << std::endl;
            }
            else {
                std::cout << "Connection failed, retrying..." << std::endl;
                UCOSM_RESTART;  // Restart from beginning
            }

            UCOSM_WAIT(200);  // Send delay

            mStateList.push_back(WAITING);
            std::cout << "Waiting for response..." << std::endl;

            mRetries = 0;
            mAttempts = 0;

            UCOSM_WAIT_UNTIL(responseReceived() || mRetries++ == 3, 100); // timeout at 300ms

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