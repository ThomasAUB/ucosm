#pragma once


// macro based coroutine inspired by protothread
//
// !!! INFO !!! 
// the current implementation needs to be implemented in ModuleKit
//

// Example

/*	

 * 
 * 
 * ///////		Coroutine_M	without context		///////////
 * 
 *  // ModuleKit<Coroutine_M>
 * 
 *	bool isReady(){
 *		// ...
 *	}
 *
 *	void myTask(){
 *
 * 		COROUTINE_MAX_CONTEXT_SIZE(0)
 * 
 * 		CR_START
 * 
 * 		// do stuff
 * 
 * 		CR_YIELD
 * 
 * 		// do stuff
 * 
 * 		WAIT_UNTIL(isReady())
 * 
 * 		// do stuff
 * 
 *		CR_END
 * 	}
 *
 *
 *
 *
 *
 */
 


#define CR_GET_HANDLE	thisTaskHandle()->get<Coroutine_M>()

// mandatory statement
#define CR_START																\
	switch(CR_GET_HANDLE->mLine){												\
	case 0:{


// stores the current line and return, will restart at this point
#define CR_YIELD																\
	}CR_GET_HANDLE->mLine = __LINE__;											\
	return;																		\
	case __LINE__:{


// yields until the condition is true, then stores the new line
#define CR_WAIT_UNTIL(condition)												\
	}CR_GET_HANDLE->mLine = __LINE__;											\
	case __LINE__:																\
	if(!(condition)){															\
		return;																	\
	}																			\
	CR_GET_HANDLE->mLine = __LINE__+1;case __LINE__+1:{


// set delay and YIELD
#define CR_WAIT_FOR(delay)														\
	thisTaskHandle()->get<Interval_M>()->setDelay(delay);						\
	CR_YIELD
	

// restarts the coroutine
#define CR_RESET																\
	CR_GET_HANDLE->mLine = 0;													\
	return;


// mandatory statement
#define CR_END         															\
	break;}																		\
	default:/* error case : should not happen */								\
	break;}																		\
	deleteTask(thisTaskHandle());


// TODO : 
//	CR_WHILE(condition) : loops and yield on every iterations
//	CR_DO and CR_WHILE(condition) : loops and yield on every iterations





struct Coroutine_M
{
	
	uint16_t mLine;

	void init() { mLine = 0; }

	bool isExeReady() const { return true; }

	bool isDelReady() const { return true; } 

	void makePreExe(){}

	void makePreDel(){}

	void makePostExe(){}
		
};


