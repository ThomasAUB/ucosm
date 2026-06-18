#include "tests.hpp"
#include "doctest.h"

#include "ulink.hpp"

TEST_CASE("ulink::List - push and size") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;

    SUBCASE("Empty list") {
        CHECK(list.empty());
        CHECK(list.size() == 0);
    }

    SUBCASE("push_front") {
        TestNode a(1), b(2), c(3);
        list.push_front(a);
        CHECK(!list.empty());
        CHECK(list.size() == 1);
        CHECK(list.front().value == 1);

        list.push_front(b);
        CHECK(list.size() == 2);
        CHECK(list.front().value == 2);
        CHECK(list.back().value == 1);

        list.push_front(c);
        CHECK(list.size() == 3);
        CHECK(list.front().value == 3);
        CHECK(list.back().value == 1);
    }

    SUBCASE("push_back") {
        TestNode a(1), b(2), c(3);
        list.push_back(a);
        CHECK(list.size() == 1);
        CHECK(list.front().value == 1);
        CHECK(list.back().value == 1);

        list.push_back(b);
        CHECK(list.front().value == 1);
        CHECK(list.back().value == 2);

        list.push_back(c);
        CHECK(list.back().value == 3);
    }
}

TEST_CASE("ulink::List - iteration") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;
    TestNode a(1), b(2), c(3);
    list.push_back(a);
    list.push_back(b);
    list.push_back(c);

    SUBCASE("Forward iteration") {
        int expected = 1;
        for (auto& node : list) {
            CHECK(node.value == expected);
            expected++;
        }
        CHECK(expected == 4);
    }
}

TEST_CASE("ulink::List - pop_front and pop_back") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;
    TestNode a(1), b(2), c(3);
    list.push_back(a);
    list.push_back(b);
    list.push_back(c);

    SUBCASE("pop_front") {
        list.pop_front();
        CHECK(list.size() == 2);
        CHECK(list.front().value == 2);
        CHECK(list.back().value == 3);

        list.pop_front();
        CHECK(list.size() == 1);
        CHECK(list.front().value == 3);

        list.pop_front();
        CHECK(list.empty());
    }

    SUBCASE("pop_back") {
        list.pop_back();
        CHECK(list.size() == 2);
        CHECK(list.front().value == 1);
        CHECK(list.back().value == 2);

        list.pop_back();
        CHECK(list.size() == 1);
        CHECK(list.front().value == 1);

        list.pop_back();
        CHECK(list.empty());
    }

    SUBCASE("pop on empty list does not crash") {
        ulink::List<TestNode> emptyList;
        emptyList.pop_front();
        emptyList.pop_back();
        CHECK(emptyList.empty());
    }
}

TEST_CASE("ulink::List - clear") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;
    TestNode a(1), b(2), c(3);
    list.push_back(a);
    list.push_back(b);
    list.push_back(c);

    list.clear();
    CHECK(list.empty());
    CHECK(list.size() == 0);

    CHECK(!a.isLinked());
    CHECK(!b.isLinked());
    CHECK(!c.isLinked());
}

TEST_CASE("ulink::List - insert_before and insert_after") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;
    TestNode a(1), b(2), c(3), d(4);

    list.push_back(a);
    list.push_back(c);

    SUBCASE("insert_after") {
        list.insert_after(list.begin(), b);
        CHECK(list.size() == 3);
        CHECK(b.value == 2);
    }

    SUBCASE("insert_before") {
        auto it = list.begin();
        ++it;
        list.insert_before(it, b);
        CHECK(list.size() == 3);
    }
}

TEST_CASE("ulink::List - erase") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;
    TestNode a(1), b(2), c(3);
    list.push_back(a);
    list.push_back(b);
    list.push_back(c);

    auto it = list.begin();
    ++it;
    list.erase(it);
    CHECK(list.size() == 2);
    CHECK(!b.isLinked());
}

TEST_CASE("ulink::List - node auto-unlink on destruction") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;

    // Use scoped nodes so there is no leak regardless of which subcase runs.
    // The "destroy linked node" subcase uses a heap node, which it deletes
    // itself; the second node here is always cleaned up at scope exit.
    TestNode b(2);
    list.push_back(b);

    CHECK(list.size() == 1);

    SUBCASE("Destroying a linked node unlinks it") {
        TestNode* node = new TestNode(1);
        list.push_front(*node);
        CHECK(list.size() == 2);
        CHECK(node->isLinked());
        delete node;
        CHECK(list.size() == 1);
        CHECK(&list.front() == &b);
    }

    // b is still linked here; it auto-unlinks when destroyed at scope exit.
    CHECK(list.size() == 1);
}

TEST_CASE("ulink::List - node remove") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;
    TestNode a(1), b(2), c(3);
    list.push_back(a);
    list.push_back(b);
    list.push_back(c);

    b.remove();
    CHECK(!b.isLinked());
    CHECK(list.size() == 2);
    CHECK(list.front().value == 1);
    CHECK(list.back().value == 3);
}

TEST_CASE("ulink::List - single element operations") {

    struct TestNode : ulink::Node<TestNode> {
        int value = 0;
        TestNode(int v = 0) : value(v) {}
    };

    ulink::List<TestNode> list;
    TestNode a(42);
    list.push_front(a);

    CHECK(list.size() == 1);
    CHECK(!list.empty());
    CHECK(list.front().value == 42);
    CHECK(list.back().value == 42);

    list.pop_front();
    CHECK(list.empty());
    CHECK(list.size() == 0);
}