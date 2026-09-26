#include "tests.hpp"
#include "doctest.h"

#include "ucosm/sync/message_queue.hpp"
#include "ucosm/sync/shared_variable.hpp"
#include <cstdint>
#include <thread>

TEST_CASE("MessageQueue - capacity and wrap-around") {

    ucosm::MessageQueue<int, 4> queue;

    static_assert(decltype(queue)::capacity == 4);

    CHECK(queue.empty());
    CHECK(queue.size() == 0);

    // fill and drain several times so that the indices wrap
    for (int round = 0; round < 5; ++round) {

        for (int i = 0; i < 4; ++i) {
            REQUIRE(queue.trySend(round * 10 + i));
        }

        CHECK(queue.full());
        CHECK(queue.size() == 4);
        CHECK_FALSE(queue.trySend(-1));

        for (int i = 0; i < 4; ++i) {
            int value = -1;
            REQUIRE(queue.tryReceive(value));
            CHECK(value == round * 10 + i);
        }

        int value = -1;
        CHECK_FALSE(queue.tryReceive(value));
        CHECK(queue.empty());
    }

    REQUIRE(queue.trySend(1));
    REQUIRE(queue.trySend(2));
    queue.clear();
    CHECK(queue.empty());

    // usable again after a clear
    REQUIRE(queue.trySend(3));
    int value = -1;
    REQUIRE(queue.tryReceive(value));
    CHECK(value == 3);
}

TEST_CASE("MessageQueue - single slot") {

    ucosm::MessageQueue<int, 1> queue;

    REQUIRE(queue.trySend(1));
    CHECK(queue.full());
    CHECK_FALSE(queue.trySend(2));

    int value = -1;
    REQUIRE(queue.tryReceive(value));
    CHECK(value == 1);
    CHECK(queue.empty());
}

TEST_CASE("MessageQueue - bulk send and receive") {

    ucosm::MessageQueue<int, 8> queue;

    const int in[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int out[10] = {};

    // only what fits is sent
    CHECK(queue.trySend(in, 10) == 8);
    CHECK(queue.full());
    CHECK(queue.trySend(in, 1) == 0);

    CHECK(queue.tryReceive(out, 5) == 5);
    for (int i = 0; i < 5; ++i) {
        CHECK(out[i] == i);
    }

    // this batch wraps past the end of the buffer
    CHECK(queue.trySend(in, 5) == 5);
    CHECK(queue.size() == 8);

    // only what is queued is received
    CHECK(queue.tryReceive(out, 10) == 8);
    const int expected[8] = { 5, 6, 7, 0, 1, 2, 3, 4 };
    for (int i = 0; i < 8; ++i) {
        CHECK(out[i] == expected[i]);
    }

    CHECK(queue.tryReceive(out, 10) == 0);
    CHECK(queue.trySend(in, 0) == 0);
    CHECK(queue.empty());
}

TEST_CASE("MessageQueue - producer and consumer threads") {

    ucosm::MessageQueue<uint32_t, 8> queue;

    constexpr uint32_t count = 100000;

    std::thread producer([&] {
        for (uint32_t i = 0; i < count; ) {
            if (queue.trySend(i)) {
                ++i;
            }
        }
    });

    uint32_t expected = 0;
    bool ordered = true;

    while (expected < count) {
        uint32_t value;
        if (queue.tryReceive(value)) {
            ordered = ordered && (value == expected);
            ++expected;
        }
    }

    producer.join();

    CHECK(ordered);
    CHECK(queue.empty());
}

TEST_CASE("MessageQueue - bulk producer and consumer threads") {

    ucosm::MessageQueue<uint32_t, 16> queue;

    constexpr uint32_t count = 100000;

    // uneven batch sizes so that the batches straddle the buffer end
    std::thread producer([&] {
        uint32_t batch[7];
        for (uint32_t next = 0; next < count; ) {
            uint32_t n = 0;
            while (n < 7 && next + n < count) {
                batch[n] = next + n;
                ++n;
            }
            next += static_cast<uint32_t>(queue.trySend(batch, n));
        }
    });

    uint32_t expected = 0;
    bool ordered = true;
    uint32_t batch[5];

    while (expected < count) {
        const auto n = queue.tryReceive(batch, 5);
        for (size_t i = 0; i < n; ++i) {
            ordered = ordered && (batch[i] == expected);
            ++expected;
        }
    }

    producer.join();

    CHECK(ordered);
    CHECK(queue.empty());
}

TEST_CASE("SharedVariable - versioning") {

    ucosm::SharedVariable<int> shared(5);

    CHECK(shared.load() == 5);

    const auto version = shared.getVersion();
    CHECK_FALSE(shared.hasChanged(version));

    shared.store(7);

    CHECK(shared.load() == 7);
    CHECK(shared.hasChanged(version));
    CHECK(shared.getVersion() == version + 1);

    // storing the same value is still an update
    const auto version2 = shared.getVersion();
    shared.store(7);
    CHECK(shared.hasChanged(version2));
}

TEST_CASE("SharedVariable - value is at least as recent as the version") {

    ucosm::SharedVariable<uint32_t> shared(0);

    constexpr uint32_t count = 100000;

    // the writer stores 1, 2, 3... so the version equals the last value
    std::thread writer([&] {
        for (uint32_t i = 1; i <= count; ++i) {
            shared.store(i);
        }
    });

    bool consistent = true;
    uint32_t version = 0;

    while (version < count) {
        version = shared.getVersion();
        consistent = consistent && (shared.load() >= version);
    }

    writer.join();

    CHECK(consistent);
    CHECK(shared.load() == count);
}
