#pragma once

// macro based coroutine inspired by protothread with context saving
//
// !!! INFO !!! 
// the current implementation needs to be implemented in ModuleHub
//

// Example

/*	
 * 
 * 
 *  // ModuleHub<Coroutine_ctx_M<myMaxContextSize>>
 * 
 *	bool isReady(){
 *		// ...
 *	}
 *
 *	CR_CTX_(myTask, thisTaskHandle())) {
 *	
 *		uint8_t k = 5;
 * 		int32_t p = 100;
 * 		 
 * 		__CR_CTX_START__ // start point
 * 
 * 		// do stuff
 * 
 * 		k++;
 * 
 * 		__CR_CTX_YIELD__ // returns and restarts at this point
 * 
 * 		// do stuff
 * 	
 * 		p++;
 * 
 * 
 *	// if isReady() returns false, returns and restarts at this point
 * 	// else continue and returns after the condition 
 * 
 * 		__CR_CTX_WAIT_UNTIL(isReady()) // 
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
 * 		__CR_CTX_RESET__
 * 
 * 
 * // deletes the task
 *		__CR_CTX_END__
 * 	}
 *
 *
 *
 *
 *
 */

// coroutine definition
#define CR_CTX(name, cr_task_handle)												\
	void name(TaskHandle inHandle){																\
	if(!cr_task_handle->mCtxLine){	cr_task_handle->instantiate<crctx_##name>(); } 	\
	crctx_##name *i = cr_task_handle->getInstance<crctx_##name>();					\
	bool endTask = false;														\
	i->run(cr_task_handle->mCtxLine, endTask);										\
	if(endTask){ deleteTask(inHandle); }}									\
	struct crctx_##name

// mandatory statement
#define __CR_CTX_START__														\
	void run(uint16_t& line, bool& end){										\
	switch(line){																\
	case 0xFFFF:/*has been reset*/												\
	case 0 : line = __LINE__ ; case __LINE__ : {

// stores the current line and returns, will restart at this point
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
	}break;																		\
	default:while(1){}/* error case : should not happen */						\
	}/*switch*/																	\
	end = true; return;															\
	}};void MAKE_UNIQUE(dummy)(){/* dummy function only used for syntax purpose*/

// TODO : 
//	CR_WHILE(condition) : loops and yield on every iterations

//#include <string.h>

template<size_t max_context_size>
struct Coroutine_ctx_M {

    static_assert(max_context_size >= 1 , "Context size must be >= 1");

    uint16_t mCtxLine;

    void init() {
        mCtxLine = 0;
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

    template<typename T>
    void instantiate() {
        static_assert(sizeof(T) <= sizeof(mContext), "Context Size Error");
        //T t;
        //memcpy(mContext, &t, sizeof(t));
        //T* t = new(mContext) T;
        new (mContext) T;
    }

    template<typename T>
    T* getInstance() {
        static_assert(sizeof(T) <= sizeof(mContext), "Context Size Error");
        return reinterpret_cast<T*>(mContext);
    }

private:

    uint8_t mContext[max_context_size];

};

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x) CONCATENATE(x, __COUNTER__)

