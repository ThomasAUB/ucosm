#pragma once


#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>

#include "../include/ucosm/task_object.h"
#include "../include/ucosm/task_function.h"
#include "../include/ucosm/modules/priority_m.h"


class PriorityTest : public CppUnit::TestFixture { 
    
    CPPUNIT_TEST_SUITE(PriorityTest);
    CPPUNIT_TEST(testPriority);
    CPPUNIT_TEST_SUITE_END();

    public:
    
        void setUp(){}

        void tearDown(){}

    protected: 

        void testPriority(){
                        
            mKernel.addTask(&mTestH);

            // while low priority has no execution
            while(!mTestH.mLowPrioCounter){
                mKernel.schedule();
            }
            // assert high priority task value
            CPPUNIT_ASSERT(mTestH.mHighPrioCounter>200);
        }

        struct TestH : public TaskFunction<TestH, 2, Priority_M>{

            TestH() : mHighPrioCounter(0), mLowPrioCounter(0)
            {
                TaskHandle h;
                // create low priority task
                if(this->createTask(&TestH::run_lowPrio, &h)){
                    h->setPriority(255);
                }
                // create high priority task
                if(this->createTask(&TestH::run_highPrio, &h)){
                    h->setPriority(1);
                }
            }

            void run_lowPrio(TaskHandle h){
                mLowPrioCounter++;
            }

            void run_highPrio(TaskHandle h){
                mHighPrioCounter++;
            }

            uint16_t mHighPrioCounter;
            uint16_t mLowPrioCounter;
        };

        TaskObject<1> mKernel;
        TestH mTestH;
        
};
