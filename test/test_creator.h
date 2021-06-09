#pragma once


#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>

#include "../include/ucosm/task_object.h"
#include "../include/ucosm/task_function.h"
#include "../include/ucosm/modules/creator_m.h"



class CreatorTest : public CppUnit::TestFixture { 
    
    CPPUNIT_TEST_SUITE(CreatorTest);
    CPPUNIT_TEST(testCreator);
    CPPUNIT_TEST_SUITE_END();

    public:
    
        void setUp(){}

        void tearDown(){}

    protected: 

        void testCreator(){
                        
            mKernel.addTask(&mTestH);

            while(mTestH.getTaskCount()){
                mKernel.schedule();
            }
        }

        using text_t =std::array<char, 50>;

        struct TestH : public TaskFunction<TestH, 5, Creator_M<text_t, 5>>{

            TestH()
            {

                TaskHandle h1, h2;
		
                createTask(&TestH::myTask, &h1);

                createTask(&TestH::myTask, &h2);

                text_t *t1 = h1->create();
                text_t *t2 = h2->create();

                CPPUNIT_ASSERT(t1);
                CPPUNIT_ASSERT(t2);

                *t1 = mAutoText;
                *t2 = mNAutoText;

                h2->setAutoRelease(false);
            }

            void myTask(TaskHandle h){

                text_t *t = h->get();

                CPPUNIT_ASSERT(t);

                if(h->isAutoRelease()){
                    CPPUNIT_ASSERT(*t == mAutoText);
                }else{
                    CPPUNIT_ASSERT(*t == mNAutoText);
                }

                if(!deleteTask(h)){
                    CPPUNIT_ASSERT(!h->isAutoRelease());
                    CPPUNIT_ASSERT(*t == mNAutoText);
                    h->destroy();
                    CPPUNIT_ASSERT(deleteTask(h));
                }

            }

            text_t mAutoText = {"Auto release task"};
            text_t mNAutoText = {"Non auto release task"};

        };

        
        TaskObject<1> mKernel;
        TestH mTestH;
        
};
