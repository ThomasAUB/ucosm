#pragma once


#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>

#include "../include/ucosm/task_object.h"
#include "../include/ucosm/task_function.h"
#include "../include/ucosm/modules/delay_m.h"

// time base
#include "time_base.h"
#include "../include/ucosm/ucosm_sys_data.h"
UcosmSysData::tick_t (*UcosmSysData::sGetTick)() = &getTick;



class DelayTest : public CppUnit::TestFixture { 
    
    CPPUNIT_TEST_SUITE(DelayTest);
    CPPUNIT_TEST(testDelay);
    CPPUNIT_TEST_SUITE_END();

    public:
    
        void setUp(){}

        void tearDown(){}

    protected: 

        void testDelay(){
                        
            mKernel.addTask(&mTestH);

            // while low priority has no execution
            while(!mTestH.mRun1Counter){
                mKernel.schedule();
            }
            // assert high priority task value
            CPPUNIT_ASSERT(mTestH.mRun2Counter==10);
        }

        struct TestH : public TaskFunction<TestH, 2, Delay_M>{

            TestH() : mRun1Counter(0), mRun2Counter(0)
            {
                TaskHandle h;
                // create low priority task
                if(this->createTask(&TestH::run_1, &h)){
                    h->setDelay(100);
                }
                // create high priority task
                if(this->createTask(&TestH::run_2, &h)){
                    h->setDelay(10);
                }
            }

            void run_1(TaskHandle h){
                mRun1Counter++;
            }

            void run_2(TaskHandle h){
                mRun2Counter++;
                h->setDelay(10);
            }

            uint16_t mRun1Counter;
            uint16_t mRun2Counter;
        };

        TaskObject<1> mKernel;
        TestH mTestH;
        
};
