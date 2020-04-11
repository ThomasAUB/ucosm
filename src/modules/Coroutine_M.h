#pragma once




// macro based coroutine
// inspired by protothread

template<uint16_t max_context_size = 0> 
struct Coroutine_M
{

#define CR_CTX_START			struct Ctx_def{

#define CR_CTX_END(label)		};Ctx_def *label = thisTaskHandle()->getContext<Ctx_def>();

#define CR_START				switch(thisTaskHandle()->mLine){case kFirstExecution:thisTaskHandle()->mLine = 0;case 0:

#define CR_YIELD				thisTaskHandle()->mLine = __LINE__;return;case  __LINE__  :

#define CR_WAIT_UNTIL(cond)		thisTaskHandle()->mLine = __LINE__;case __LINE__ :  if(!(cond)){return;}

#define CR_WAIT_FOR(delay)		thisTaskHandle()->get<Delay>().setDelay(delay);CR_YIELD // set delay and YIELD

#define CR_LOOP(cond)			for(;(cond);CR_YIELD)

#define CR_RESET				thisTaskHandle()->mLine = 0;return;
	
#define CR_END         			}deleteTask(thisTaskHandle());


	
	template<typename T>
	T *getContext()
	{
		static_assert(sizeof(T) <= sizeof(mContext), "Coroutine context size error");
		if(mLine == kFirstExecution){
			// instantiate T inside context buffer
			T temp;
			uint8_t *dest = mContext;
			uint8_t *src = reinterpret_cast<uint8_t *>(&temp);
			const uint8_t *end = src+sizeof(T);
			while(src != end){
				*dest++ = *src++;
			}
		}
		return reinterpret_cast<T *>(mContext);
	}

	uint16_t mLine;

	template<typename T>
	void init(T *t) { mLine = kFirstExecution;}
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
	
	uint8_t mContext[max_context_size];
	static const uint16_t kFirstExecution = 0xFFFF;
};

