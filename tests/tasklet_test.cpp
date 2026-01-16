#include "tests.hpp"
#include "doctest.h"

#include "ucosm/tasklet/tasklet_scheduler.hpp"
#include "desktop_tasklet_executor.hpp"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

TEST_CASE("TaskletScheduler - timer wrap-around behavior") {

    using namespace ucosm;

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

    // schedule t1 to wake shortly after wrap (e.g., +10), t2 a bit later (+20)
    t1.sleepFor(10);
    t2.sleepFor(20);

    REQUIRE(sched.addTask(t1));
    REQUIRE(sched.addTask(t2));

    // advance across the wrap boundary in small steps so timer expiry is detected
    uint64_t start = static_cast<uint64_t>(UINT32_MAX) - 2;
    // iterate enough ticks across the wrap so both deadlines (5 and 15) are reached
    for (uint64_t v = start; v < start + 40; ++v) {
        sched.tick(static_cast<uint32_t>(v));
    }

    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&] { return executed.size() == 2; });
    }

    REQUIRE(executed.size() == 2);
    CHECK(executed[0] == 1);
    CHECK(executed[1] == 2);

}

TEST_CASE("TaskletScheduler - concurrent signalInterrupt calls") {

    using namespace ucosm;

    ucosm::TaskletScheduler<4> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct ISRTask : ucosm::ITasklet {
        ISRTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
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

    ISRTask a(1, &executed, &m, &cv);
    ISRTask b(2, &executed, &m, &cv);
    ISRTask c(3, &executed, &m, &cv);

    a.setPriority(10);
    b.setPriority(5);
    c.setPriority(15);

    a.waitForInterrupt(0);
    b.waitForInterrupt(1);
    c.waitForInterrupt(2);

    REQUIRE(sched.addTask(a));
    REQUIRE(sched.addTask(b));
    REQUIRE(sched.addTask(c));

    // Suspend low-priority execution while we concurrently set pending interrupts
    suspend_low_priority_execution();

    std::thread th1([&] { sched.signalInterrupt(0); });
    std::thread th2([&] { sched.signalInterrupt(1); });
    std::thread th3([&] { sched.signalInterrupt(2); });

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

TEST_CASE("TaskletScheduler - combined sleep and ISR ordering") {

    using namespace ucosm;

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

    MixedTask sleepMid(1, &executed, &m, &cv); // mid priority but sleeps
    MixedTask isrHigh(2, &executed, &m, &cv);    // high priority, ISR
    MixedTask isrLow(3, &executed, &m, &cv);    // low priority, ISR

    sleepMid.setPriority(10);
    isrHigh.setPriority(5); // high priority
    isrLow.setPriority(20);

    // sleepHigh will wake at tick 50
    sleepMid.sleepFor(50);
    isrHigh.waitForInterrupt(0);
    isrLow.waitForInterrupt(1);

    REQUIRE(sched.addTask(sleepMid));
    REQUIRE(sched.addTask(isrHigh));
    REQUIRE(sched.addTask(isrLow));

    // trigger both ISRs first; they should run (ordered by priority among ISRs)
    sched.tick(50);
    sched.signalInterrupt(0);
    sched.signalInterrupt(1);

    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&] { return executed.size() >= 2; });
    }

    // So far, executed should contain isrMid (priority 10) then isrLow (20)
    CHECK(executed.size() == 3);
    CHECK(executed[0] == 2);
    CHECK(executed[1] == 1);
    CHECK(executed[2] == 3);

}

TEST_CASE("TaskletScheduler - interrupt ordering") {

    using namespace ucosm;

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

    t1.waitForInterrupt(0);
    t2.waitForInterrupt(1);
    t3.waitForInterrupt(2);

    REQUIRE(sched.addTask(t1));
    REQUIRE(sched.addTask(t2));
    REQUIRE(sched.addTask(t3));

    // signal all interrupts; run handler asynchronously
    sched.signalInterrupt(0);
    sched.signalInterrupt(1);
    sched.signalInterrupt(2);

    // wait for tasks to run
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, std::chrono::seconds(2), [&] { return executed.size() == 3; });
    }

    CHECK(executed.size() == 3);

    // expected order by priority ascending: t2 (5), t1 (10), t3 (20)
    CHECK(executed[0] == 2);
    CHECK(executed[1] == 1);
    CHECK(executed[2] == 3);

}

TEST_CASE("TaskletScheduler - timer wake ordering") {

    using namespace ucosm;

    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    std::vector<int> executed;
    std::mutex m;
    std::condition_variable cv;

    struct SleepTask : ucosm::ITasklet {
        SleepTask(int id, std::vector<int>* out, std::mutex* m, std::condition_variable* cv) :
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

    SleepTask s1(1, &executed, &m, &cv);
    SleepTask s2(2, &executed, &m, &cv);
    SleepTask s3(3, &executed, &m, &cv);

    // set sleep durations so they wake in order 2,1,3
    s1.sleepFor(50);
    s2.sleepFor(10);
    s3.sleepFor(100);

    // add tasks (they will be inserted into the timer list)
    REQUIRE(sched.addTask(s1));
    REQUIRE(sched.addTask(s2));
    REQUIRE(sched.addTask(s3));

    sched.tick(10);

    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&] { return executed.size() >= 1; });
    }
    CHECK(executed.size() == 1);
    CHECK(executed[0] == 2);

    // advance to 50 -> should wake s1
    sched.tick(50);

    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&] { return executed.size() >= 2; });
    }
    CHECK(executed.size() == 2);
    CHECK(executed[1] == 1);

    // advance to 100 -> should wake s3
    sched.tick(100);

    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&] { return executed.size() >= 3; });
    }
    CHECK(executed.size() == 3);
    CHECK(executed[2] == 3);

}
