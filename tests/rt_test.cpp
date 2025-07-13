#include "tests.hpp"

#include "doctest.h"

#include "ucosm/rt/rt_scheduler.hpp"

#include <chrono>
#include <iostream>

static uint32_t getMS() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void rtTaskTests() {

    struct Timer : ucosm::RTScheduler::ITimer {

        void start() override {
            mIsStarted = true;
        }

        void stop() override {
            mIsStarted = false;
        }

        bool isRunning() const override {
            return mIsStarted;
        }

        void setDuration(uint32_t inDuration) override {
            mDuration = inDuration;
        }

        void disableInterruption() override {

        }

        void enableInterruption() override {

        }

        void runTimer() {

            auto start = getMS();

            while (mIsStarted) {

                if (start != getMS()) {
                    // increase of at least 1 ms

                    // increase counter
                    mCounter += getMS() - start;

                    start = getMS();

                }

                if (mCounter >= mDuration) {
                    //mCounter -= mDuration;
                    mCounter = 0;
                    this->processIT();
                }
            }
        }

    private:
        bool mIsStarted = false;
        uint32_t mCounter = 0;
        uint32_t mDuration = 0;
    };


    struct RTTask : ucosm::IPeriodicTask {

        RTTask(int id, uint32_t inPeriod) :
            ucosm::IPeriodicTask(inPeriod), mID(id) {
            mPrevTimeStamp = getMS();
        }

        void run() override {

            //if (mFirst) {
            //    std::cout << "run task " << mID << " first" << std::endl;
            //    mFirst = false;
            //}
            //else {
            std::cout << "run task " << mID << " time : " << getMS() - mPrevTimeStamp << std::endl;
            //}

            mPrevTimeStamp = getMS();

            // simulate work
            const auto start = getMS();
            while (getMS() - start != 100);

            if (mCounter++ == 5) {
                this->removeTask();
            }

        }
        bool mFirst = true;
        uint32_t mPrevTimeStamp = 0;
        int mCounter = 0;
        int mID;
    };

    RTTask task1(1, 1111);
    RTTask task2(2, 2424);

    Timer tim;

    ucosm::RTScheduler sched(tim);

    sched.addTask(task1);
    sched.addTask(task2);

    tim.runTimer();
}