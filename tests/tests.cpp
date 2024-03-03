#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "ucosm/itask.hpp"
#include "ucosm/scheduler.hpp"

uint32_t getMillis() {
    static auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

struct DelayTask : ucosm::ITask<uint32_t> {

    bool isReady() const override {
        return getMillis() >= this->getRank();
    }

    void setDelay(uint32_t inDelay) {
        this->setRank(getMillis() + inDelay);
    }

};

struct PeriodicTask : DelayTask {

    PeriodicTask(uint32_t inPeriod) :
        mPeriod(inPeriod) {}

    void setPeriod(uint32_t inPeriod) {
        mPeriod = inPeriod;
    }

    virtual void periodCallback() = 0;

    void run() override {
        periodCallback();
        this->setDelay(mPeriod);
    }

    uint32_t mPeriod;

};

struct Scheduler : ucosm::IScheduler<uint32_t> {

    //bool isReady() const override {
    //    return getMillis() >= this->getRank();
    //}

    void setDelay(uint32_t inDelay) {
        this->setRank(getMillis() + inDelay);
    }

};

struct CFS : ucosm::IScheduler<uint16_t> {



};

TEST_CASE("basic ucosm tests") {

    struct MyTask : DelayTask {

        void run() override {

            //if (mCounter++ == 10) {
            //    this->remove();
            //}

            std::cout << "hello" << std::endl;
            this->setDelay(1000);
        }

    };

    struct MyOtherTask : PeriodicTask {

        MyOtherTask() : PeriodicTask(3000) {}

        void periodCallback() override {
            if (mCounter++ == 5) {
                this->setPeriod(1);
            }
            else if (mCounter == 20) {
                mCounter = 0;
                this->setPeriod(3000);
            }
            std::cout << "coucou" << std::endl;

        }
        uint8_t mCounter = 0;
    };

    Scheduler sched;

    MyTask t1;
    MyOtherTask t2;

    sched.addTask(t1);
    sched.addTask(t2);
    /*
        for (std::size_t i = 0; i < 10000000; i++) {
            sched.run();
            sCounter++;
        }
    */
    while (!sched.empty()) {
        sched.run();
    }

    CHECK(1);


}

