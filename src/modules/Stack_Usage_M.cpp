#include "Stack_Usage_M.h"


template<uint16_t max_stack_usage>
uint32_t *Stack_Usage_M<max_stack_usage>sSp;




template<uint16_t max_stack_usage>
void Stack_Usage_M<max_stack_usage>::makePreExe(){

	if(!mIsRunning){
		return;
	}

	const auto size=max_stack_usage/sizeof(uint32_t);
	uint32_t s[size];

	sSp = s;

	uint16_t i=0;
	while(i < size){
		s[i++] = kStackPattern;
	}

}

template<uint16_t max_stack_usage>
void Stack_Usage_M<max_stack_usage>::makePostExe(){
	volatile uint8_t k=5;
	if(!mIsRunning){
		return;
	}	

	const auto size=max_stack_usage/sizeof(uint32_t);
	uint32_t s[size];

	if(sSp != s){
		//stack overflow or memory leakage	
		mStackUsage = 0xFFFF;
		stop();
		return;
	}

	uint16_t i=0;

	while(i < size){
		if(s[i] == kStackPattern){
			// stack usage
			mStackUsage = i*sizeof(uint32_t);
			if(mStackUsage > mMaxStackUsage){
				mMaxStackUsage = mStackUsage;
			}
			return;
		}
		i++;
	}
	mStackUsage = 0xFFFF;
	stop();
}



	



