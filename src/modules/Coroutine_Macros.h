#pragma once
 

#include <limits>

using cr_line_t = uint16_t;
const uint16_t kFirst_CR_Execution = std::numeric_limits<cr_line_t>::max();






#define COROUTINE_MAX_CONTEXT_SIZE(size)										\
	using cr_internal_t = Coroutine_M<size>;


#define CR_GET_HANDLE	thisTaskHandle()->get<cr_internal_t>()


// context
#define CR_CTX_START															\
	struct Ctx_def{

#define CR_CTX_END(label)														\
	};																			\
	Ctx_def label = CR_GET_HANDLE->getContext<Ctx_def>();




// mandatory statement
#define CR_START																\
	switch(CR_GET_HANDLE->mLine){												\
	case kFirst_CR_Execution:													\
		CR_GET_HANDLE->mLine = 0;												\
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




