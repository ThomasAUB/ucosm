#pragma once


#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>

#include "../include/ucosm/modules/utils/MemPool_32.h"

#include <cstring>
#include <array>

class MemPool32Test : public CppUnit::TestFixture { 
    
    CPPUNIT_TEST_SUITE(MemPool32Test);
    CPPUNIT_TEST(testPool32);
    CPPUNIT_TEST_SUITE_END();

    public:
    
        void setUp(){}

        void tearDown(){}

    protected: 

        static const uint8_t kTextSize = 50;

        void testPool32(){

            MemPool_32<3, 100> mem;

            TextObject *o1 = nullptr;
            TextObject *o2 = nullptr;
            TextObject *o3 = nullptr;
            TextObject *o4 = nullptr;

            std::array<char, kTextSize> t1 = {"My text 1"};
            std::array<char, kTextSize> t2 = {"My text 2"};

            if(mem.allocate(&o1, t1.begin())){
		        CPPUNIT_ASSERT(o1 != nullptr);
                CPPUNIT_ASSERT(o1->text == t1);
            }else{
                CPPUNIT_ASSERT(false);
            }

            if(mem.allocate(&o2, t2.begin())){
		        CPPUNIT_ASSERT(o2 != nullptr);
                CPPUNIT_ASSERT(o2->text == t2);
            }else{
                CPPUNIT_ASSERT(false);
            }

            if(mem.allocate(&o3)){
		        CPPUNIT_ASSERT(o3 != nullptr);
                CPPUNIT_ASSERT(o3->text[0] == 'A');
            }else{
                CPPUNIT_ASSERT(false);
            }
            
            if(mem.allocate(&o4)){
		        CPPUNIT_ASSERT(false);
            }else{
                CPPUNIT_ASSERT(true);
            }

            CPPUNIT_ASSERT(mem.getSizeLeft() == 0);

            if(mem.release(&o1)){
		        CPPUNIT_ASSERT(o1 == nullptr);
            }else{
                CPPUNIT_ASSERT(false);
            }

            if(mem.release(&o2)){
		        CPPUNIT_ASSERT(o2 == nullptr);
            }else{
                CPPUNIT_ASSERT(false);
            }

            if(mem.release(&o3)){
		        CPPUNIT_ASSERT(o3 == nullptr);
            }else{
                CPPUNIT_ASSERT(false);
            }

            if(mem.release(&o4)){
                CPPUNIT_ASSERT(false);
            }

            CPPUNIT_ASSERT(mem.getSizeLeft() == 3);
        }

        struct TextObject{
            
            TextObject(){
                text[0] = 'A';
            }
            TextObject(char* t){
                strncpy(text.begin(), t, kTextSize);
                text[kTextSize-1] = 0;
            }
            std::array<char,kTextSize> text;
        };
        
        
};
