#pragma once


// macro based coroutine inspired by protothread with context saving
//
// !!! INFO !!! 
// the current implementation needs to be implemented in ModuleKit
//

// Example

/*	
 * 
 * 
 *  // ModuleKit<Coroutine_ctx_M<myMaxContextSize>>
 * 
 *	bool isReady(){
 *		// ...
 *	}
 *
 *	CR_CTX_(myTask, myMaxContextSize) {
 *	
 *		uint8_t k = 5;
 * 		int32_t p = 100;
 * 		 
 * 	__CR_CTX_START__ // start point
 * 
 * 		// do stuff
 * 
 * 		k++;
 * 
 * 	__CR_CTX_YIELD__ // returns and restarts at this point
 * 
 * 		// do stuff
 * 	
 * 		p++;
 * 
 * 
 *	// if isReady() returns false, returns and restarts at this point
 * 	// else continue and returns after the condition 
 * 
 * 	__CR_CTX_WAIT_UNTIL(isReady()) // 
 * 
 * 		// do stuff
 * 
 * 		if(p-- > 1){
 *			return;
 *	 	}
 * 
 * 		k = 0;
 * 		p = 0;
 * 		
 * // restarts the function from the beginning 
 * 	__CR_CTX_RESET__
 * 
 * 
 * // deletes the task
 *	__CR_CTX_END__
 * 	}
 *
 *
 *
 *
 *
 */

// coroutine definition
#define CR_CTX_(name, max_size)													\
	void name(){																\
	using crctx_t = Coroutine_ctx_M<max_size>;									\
	crctx_t *handle = thisTaskHandle()->get<crctx_t>();							\
	if(!handle->mLine){	handle->instantiate<crctx_##name>(); } 					\
	crctx_##name *i = handle->getInstance<crctx_##name>();						\
	bool endTask = false;														\
	i->run(handle->mLine, endTask);												\
	if(endTask){ deleteTask(thisTaskHandle()); }}								\
	struct crctx_##name


// mandatory statement
#define __CR_CTX_START__														\
	void run(uint16_t& line, bool& end){										\
	switch(line){																\
	case 0xFFFF:/*has been reset*/												\
	case 0 : line = __LINE__ ; case __LINE__ : {


// stores the current line and return, will restart at this point
#define __CR_CTX_YIELD__														\
	} line = __LINE__ ; return ; case __LINE__ : {
												


// yields until the condition is true, then stores the new line
#define __CR_CTX_WAIT_UNTIL(condition)											\
	}line = __LINE__; case __LINE__:											\
	if(!(condition)){ return; }													\
	line = __LINE__ +1; case __LINE__ +1: {


// restarts the coroutine
#define __CR_CTX_RESET__														\
	line = 0xFFFF; return;
	

// mandatory statement
#define __CR_CTX_END__     														\
	}default:while(1){}/* error case : should not happen */						\
	break;}/*switch*/															\
	end = true; return;															\
	}};void dummy(){


// TODO : 
//	CR_WHILE(condition) : loops and yield on every iterations
//	CR_DO and CR_WHILE(condition) : loops and yield on every iterations


#include <string.h>



template<size_t max_context_size>
struct Coroutine_ctx_M
{
	
	uint16_t mLine;

	void init() { mLine = 0; }

	bool isExeReady() const { return true; }

	bool isDelReady() const { return true; } 

	void makePreExe(){}

	void makePreDel(){}

	void makePostExe(){}

	template<typename T>
	void instantiate(){
		static_assert(sizeof(T) <= sizeof(mContext), "Context Size Error");
		T t;
		memcpy(mContext, &t, sizeof(t));
	}
	
	template<typename T>
	T *getInstance(){
		static_assert(sizeof(T) <= sizeof(mContext), "Context Size Error");
		return reinterpret_cast<T *>(mContext);
	}
	
private:

	uint8_t mContext[max_context_size];
		
};


