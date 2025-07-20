#include "tests.hpp"
#include "doctest.h"

#include "ucosm/cfs/cfs_scheduler.hpp"

#include <iostream>

#include "doctest.h"

TEST_CASE("CFS task test") {

    struct Task : ucosm::ICFSTask {

        Task(int id, uint32_t inWorkDuration_ms) :
            mID(id),
            mWork_ms(inWorkDuration_ms) {}

        void run() override {

            auto currentTime = getMillis();

            if (mFirstExecution) {
                mFirstExecution = false;
            }
            else {
                uint32_t period = currentTime - mLastExecution;
                mAveragePeriod += period;

                std::cout
                    << "Task " << mID << " "
                    << "| period =" << period << "ms "
                    << std::endl;

            }

            mLastExecution = currentTime;

            waitFor_ms(mWork_ms);

            if (mCounter++ == count) {
                this->removeTask();
            }
        }

        uint32_t period() const {
            if (mCounter < 2) {
                return 0;
            }
            return mAveragePeriod / (mCounter - 1);
        }

        uint8_t count = 0;

    private:
        bool mFirstExecution = true;
        uint32_t mLastExecution;
        uint32_t mAveragePeriod = 0;
        uint8_t mCounter = 0;
        uint32_t mWork_ms;
        int mID;
    };

    ucosm::CFSScheduler sched(getMicros);

    Task t1(1, 10);
    Task t2(2, 10);

    CHECK(sched.size() == 0);
    CHECK(sched.empty());

    {
        Task t3(3, 500);
        sched.addTask(t3);
        CHECK(sched.size() == 1);
    }

    CHECK(sched.size() == 0);
    CHECK(sched.empty());

    t1.count = 6;
    t1.setPriority(4);

    t2.count = 10;
    t2.setPriority(6);

    sched.addTask(t1);
    sched.addTask(t2);

    CHECK(sched.size() == 2);
    CHECK(!sched.empty());

    std::cout << "\n=== CFS Scheduler start ===\n" << std::endl;

    while (!sched.empty()) {
        sched.run();
    }

    CHECK(t1.period() < t2.period());

    std::cout << "Task 1 period: " << t1.period() << "ms" << std::endl;
    std::cout << "Task 2 period: " << t2.period() << "ms" << std::endl;

    std::cout << "\n=== CFS Scheduler end ===\n" << std::endl;

}