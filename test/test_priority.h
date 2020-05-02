#ifndef TEST_UCOSM_PRIORITY_H
#define TEST_UCOSM_PRIORITY_H

#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>

#include "kernel.h"
#include "traits.h"



class PriorityTest : public CppUnit::TestFixture { 
    CPPUNIT_TEST_SUITE(PriorityTest);
    CPPUNIT_TEST(testDummy);
    CPPUNIT_TEST_SUITE_END();

    public:
        void setUp(){
        }

        void tearDown(){
        }

    protected: 
        void testDummy(){
            CPPUNIT_ASSERT(true);
        }
};

#endif //TEST_UP_MESSAGE_H