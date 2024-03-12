![build status](https://github.com/ThomasAUB/ucosm/actions/workflows/build.yml/badge.svg)
[![License](https://img.shields.io/github/license/ThomasAUB/ucosm.svg)](LICENSE)

# uCosm

Lightweight scheduler for microcontrollers.

# Example

```cpp
#include <chrono>

uint32_t getMilliseconds() {
    static auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

#include "ucosm/itask.hpp"
#include "ucosm/scheduler.hpp"

struct IPeriodicTask : ucosm::ITask<uint32_t> {

    IPeriodicTask(uint32_t inPeriod) :
        mPeriod(inPeriod) {}

protected:

    uint32_t mPeriod = 0;

private:

    virtual void periodicCallback() = 0;

    void run() final {
        periodicCallback();
        this->setRank(getMilliseconds() + mPeriod);
    }

};

struct Task : IPeriodicTask {

    Task(int inID, uint32_t inPeriod) :
        IPeriodicTask(inPeriod),
        id(inID) {}

    void periodicCallback() override {
        std::cout << "Task " << id << std::endl;
    }

private:
    int id;
};




int main() {

    Task t1(1, 1000); // execute task 1 every seconds
    Task t2(2, 2000); // execute task 2 every 2 seconds
    Task t3(3, 3000); // execute task 3 every 3 seconds

    ucosm::Scheduler<uint32_t, uint32_t> scheduler(
        +[] (uint32_t inTimeStamp) {
            return getMilliseconds() >= inTimeStamp;
        }
    );

    scheduler.addTask(t1);
    scheduler.addTask(t2);
    scheduler.addTask(t3);

    while (true) {
        scheduler.run();
    }

    return 0;
}

```
