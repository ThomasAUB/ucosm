#pragma once


#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>

#include "../include/ucosm/task_object_allocator.h"
#include "../include/ucosm/task_function.h"

class TaskObjectAllocTest : public CppUnit::TestFixture { 
    
    CPPUNIT_TEST_SUITE(TaskObjectAllocTest);
    CPPUNIT_TEST(testTaskObjectAlloc);
    CPPUNIT_TEST_SUITE_END();

    public:
    
        void setUp(){}

        void tearDown(){}

    protected: 

        void testTaskObjectAlloc(){
            TaskObjectAllocator<1, 512, void_M, 1> toa;
            TestH *th;
            CPPUNIT_ASSERT(toa.createTask(&th));
            //CPPUNIT_ASSERT(th->getTaskCount() == 0);
            //CPPUNIT_ASSERT(toa.deleteTask(&th));
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

        struct TestH : public TaskFunction<TestH, 1, Test_M>{

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

                CPPUNIT_ASSERT(deleteTask(h));
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
