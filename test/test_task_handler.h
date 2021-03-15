#pragma once


#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>

#include "../include/ucosm/kernel.h"
#include "../include/ucosm/task-handler.h"

/*
// time base
#include "time_base.h"
#include "../include/modules/ucosm-sys-data.h"
tick_t (*SysKernelData::sGetTick)() = &getTick;
*/


class TaskHandlerTest : public CppUnit::TestFixture { 
    
    CPPUNIT_TEST_SUITE(TaskHandlerTest);
    CPPUNIT_TEST(testTaskHandler);
    CPPUNIT_TEST_SUITE_END();

    public:
    
        void setUp(){}

        void tearDown(){}

    protected: 

        void testTaskHandler(){
            TestH th;
        }

        struct Test_M{
            void init(){}
            bool isExeReady() const {return true;}
            bool isDelReady() const {return true;}
            void makePreExe(){}
            void makePreDel(){
                if(mDBGIndex == 2){
                    mDBGIndex = -2;
                }
            }
            void makePostExe(){mDBGIndex=-1;}
            int16_t mDBGIndex;
        };

        struct TestH : public TaskHandler<TestH, 1, Test_M>{

            TestH(){

                TaskHandle h;

                CPPUNIT_ASSERT(this->createTask(&TestH::run_1, &h));

                h->mDBGIndex = 1;

                CPPUNIT_ASSERT(this->schedule());

                // assert the task handle has been destroyed
                CPPUNIT_ASSERT(!h());

                CPPUNIT_ASSERT(this->schedule());

                CPPUNIT_ASSERT(this->getTaskCount() == 0);

                // task creation is possible
                CPPUNIT_ASSERT(this->createTask(&TestH::run_1, &h));

                // run 2 makeDel has been called
                CPPUNIT_ASSERT(h->mDBGIndex == -2);
            }

            void run_1(TaskHandle h){

                // check if the value has been kept
                CPPUNIT_ASSERT(h->mDBGIndex == 1);

                CPPUNIT_ASSERT(deleteTask(h));

                CPPUNIT_ASSERT(!h());

                CPPUNIT_ASSERT(this->createTask(&TestH::run_2, &h));

                // assert makePostExe hasn't been called
                CPPUNIT_ASSERT(h->mDBGIndex == 1);

                h->mDBGIndex = 2;
            }

            void run_2(TaskHandle h){

                CPPUNIT_ASSERT(h->mDBGIndex == 2);

                CPPUNIT_ASSERT(deleteTask(h));

                CPPUNIT_ASSERT(!h());
            }

        };
        
};
