#include "tests.hpp"
#include "doctest.h"

#include "ucosm/tasklet/tasklet_queue.hpp"
#include "desktop_tasklet_executor.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace {

    constexpr ucosm::event_id_t rx_event = 1;

    using Queue = ucosm::TaskletQueue<int, 8, 2>;

    // Drains the queue each time it is woken up, as a tasklet fed by an ISR
    // would.
    struct ConsumerTask : ucosm::ITasklet {

        explicit ConsumerTask(Queue& inQueue) : mQueue(inQueue) {}

        void run() override {
            int value;
            while (mQueue.tryReceive(value)) {
                std::lock_guard<std::mutex> lock(mMutex);
                mReceived.push_back(value);
            }
            mRuns++;
            mCV.notify_one();
        }

        bool waitFor(std::size_t inCount) {
            std::unique_lock<std::mutex> lock(mMutex);
            return mCV.wait_for(lock, std::chrono::seconds(2),
                [&] { return mReceived.size() >= inCount; });
        }

        Queue& mQueue;
        std::mutex mMutex;
        std::condition_variable mCV;
        std::vector<int> mReceived;
        std::atomic<int> mRuns { 0 };
    };

}

TEST_CASE("TaskletQueue - a send wakes the tasklet") {

    reset_tick();
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    Queue queue(sched, rx_event);
    CHECK(queue.getEventID() == rx_event);

    ConsumerTask consumer(queue);
    consumer.waitForEvent(rx_event);
    REQUIRE(sched.addTask(consumer));

    REQUIRE(queue.trySend(42));
    REQUIRE(consumer.waitFor(1));

    {
        std::lock_guard<std::mutex> lock(consumer.mMutex);
        CHECK(consumer.mReceived == std::vector<int> { 42 });
    }

    sched.removeTask(consumer);
}

TEST_CASE("TaskletQueue - messages from an ISR-like thread arrive in order") {

    reset_tick();
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    Queue queue(sched, rx_event);

    ConsumerTask consumer(queue);
    consumer.waitForEvent(rx_event);
    REQUIRE(sched.addTask(consumer));

    constexpr int count = 1000;

    std::thread producer([&] {
        for (int i = 0; i < count; ) {
            if (queue.trySend(i)) {
                ++i;
            }
        }
    });

    producer.join();
    REQUIRE(consumer.waitFor(count));

    {
        std::lock_guard<std::mutex> lock(consumer.mMutex);
        REQUIRE(consumer.mReceived.size() == count);
        bool ordered = true;
        for (int i = 0; i < count; ++i) {
            ordered = ordered && (consumer.mReceived[i] == i);
        }
        CHECK(ordered);
    }

    // coalesced wake-ups : never more runs than messages
    CHECK(consumer.mRuns.load() <= count);

    sched.removeTask(consumer);
}

TEST_CASE("TaskletQueue - a bulk send wakes the tasklet") {

    reset_tick();
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    Queue queue(sched, rx_event);

    ConsumerTask consumer(queue);
    consumer.waitForEvent(rx_event);
    REQUIRE(sched.addTask(consumer));

    const int values[3] = { 7, 8, 9 };
    REQUIRE(queue.trySend(values, 3) == 3);
    REQUIRE(consumer.waitFor(3));

    {
        std::lock_guard<std::mutex> lock(consumer.mMutex);
        CHECK(consumer.mReceived == std::vector<int> { 7, 8, 9 });
    }

    sched.removeTask(consumer);
}

TEST_CASE("TaskletQueue - a full queue refuses the message") {

    reset_tick();
    ucosm::TaskletScheduler<2> sched(tasklet_backend);

    // no tasklet subscribed : nothing drains the queue
    Queue queue(sched, rx_event);

    for (std::size_t i = 0; i < Queue::capacity; ++i) {
        REQUIRE(queue.trySend(static_cast<int>(i)));
    }

    CHECK(queue.full());
    CHECK_FALSE(queue.trySend(-1));
    CHECK(queue.size() == Queue::capacity);
}
