/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2020 Thomas AUBERT                                                *
 *                                                                                 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy    *
 * of this software and associated documentation files (the "Software"), to deal   *
 * in the Software without restriction, including without limitation the rights    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is           *
 * furnished to do so, subject to the following conditions:                        *
 *                                                                                 *
 * The above copyright notice and this permission notice shall be included in all  *
 * copies or substantial portions of the Software.                                 *
 *                                                                                 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 * SOFTWARE.                                                                       *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

// macro based coroutine inspired by protothread
//

// Example

/*	

 * 
 * 
 * ///////		Coroutine_M		///////////
 * 
 * 
 *	bool isReady(){
 *		// ...
 *	}
 *
 *	void myTask(TaskHandle h){
 *
 * 
 * 		__CR_START(h)
 * 
 * 		// do stuff
 * 
 * 		__CR_YIELD__
 * 
 * 		// do stuff
 * 
 * 		__CR_WAIT_UNTIL(isReady())
 * 
 * 		// do stuff
 * 
 *		__CR_END(h)
 * 	}
 *
 *
 *
 *
 *
 */

// mandatory statement
#define __CR_START(inHandle)													\
	uint16_t& cr_line = inHandle->mLine;										\
	switch(cr_line){case 0:{

// stores the current line and return, will restart at this point
#define __CR_YIELD__															\
	}cr_line = __LINE__;return;case __LINE__:{

// yields until the condition is true, then stores the new line
#define __CR_WAIT_UNTIL(condition)												\
	}cr_line = __LINE__;case __LINE__:if(!(condition)){return;}					\
	cr_line = __LINE__+1;case __LINE__+1:{

///////////// do while ////////////////
#define __CR_DO__																\
	}cr_line = __LINE__;case __LINE__:

#define __CR_WHILE(condition)													\
	if((condition)){return;}													\
	cr_line = __LINE__;case __LINE__:{
//////////////////////////////////////

// restarts the coroutine
#define __CR_RESET__															\
	cr_line = 0;return;

// mandatory statement
#define __CR_END(inHandle)         												\
	this->deleteTask(inHandle);                                                 \
	break;}																		\
	default:/* error case : should not happen */								\
	break;}

// TODO : 
//	CR_WHILE(condition) : loops and yield on every iterations

struct Coroutine_M {

    uint16_t mLine;

    void init() {
        mLine = 0;
    }

    bool isExeReady() const {
        return true;
    }

    bool isDelReady() const {
        return true;
    }

    void makePreExe() {
    }

    void makePreDel() {
    }

    void makePostExe() {
    }

};

