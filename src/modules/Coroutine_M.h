#pragma once




// macro based coroutine
// inspired by protothread



/*	
 *	bool isReady(){
 *		// ...
 *	}
 *
 *	void myTask(){
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
 */
 
// if you use Coroutine_M alone (i.e. without using ModuleKit_M)
// you will have to define CR_ONLY
														
#if defined(CR_ONLY)
	#define CR_GET_HANDLE 	thisTaskHandle()							
#else
	#define CR_GET_HANDLE	thisTaskHandle()->get<Coroutine_M>()										
#endif																			


#define CR_CTX_START															\
	struct Ctx_def{

#define CR_CTX_END(label)														\
	};																			\
	Ctx_def *label = CR_GET_HANDLE->getContext<Ctx_def>();


// mandatory statement
#define CR_START																\
	switch(CR_GET_HANDLE->mLine){												\
	case 0:{


// stores the current line and return, will restart at this point
#define CR_YIELD																\
	}CR_GET_HANDLE->mLine = __LINE__;											\
	return;																		\
	case __LINE__:{


// yields until the condition is true, then stores the new point
#define CR_WAIT_UNTIL(condition)												\
	}CR_GET_HANDLE->mLine = __LINE__;											\
	case __LINE__:																\
	if(!(condition)){															\
		return;																	\
	}																			\
	CR_GET_HANDLE->mLine = __LINE__+1;case __LINE__+1:{


// set delay and YIELD
#define CR_WAIT_FOR(delay)														\
	thisTaskHandle()->get<Interval_M>()->setDelay(delay);							\
	CR_YIELD

// loops while condition is true and yields on every iterations
/*#define CR_WHILE(condition)														\
	}CR_GET_HANDLE->mLine = __LINE__;											\
	case __LINE__:{																\
	while(condition){
		return;
		{
			// client's code
		}*/

//#define CR_DO_WHILE(condition)


	

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



struct Coroutine_M
{

	
	/*template<typename T>
	T *getContext()
	{
		//static_assert(sizeof(T) <= sizeof(mContext), "Coroutine context size error");
		/*if(mLine == kFirstExecution){
			// instantiate T inside context buffer
			T temp;
			uint8_t *dest = mContext;
			uint8_t *src = reinterpret_cast<uint8_t *>(&temp);
			const uint8_t *end = src+sizeof(T);
			while(src != end){
				*dest++ = *src++;
			}
		}*/
		//return reinterpret_cast<T *>(mContext);
	//}

	uint16_t mLine;
	//static const uint16_t kFirstExecution = 0xFFFF;

	template<typename T>
	void init(T *t) { mLine = 0;}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}

private:

	//static const uint8_t max_context_size;
	//uint8_t mContext[max_context_size];
	
};


