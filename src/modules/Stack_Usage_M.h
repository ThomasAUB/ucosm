#pragma once

/*
 *	template<uint16_t max_stack_usage>
 *	uint32_t *Stack_Usage_M<max_stack_usage>::sSp; 
 */


template<uint16_t max_stack_usage>
struct Stack_Usage_M
{
	static_assert(max_stack_usage > sizeof(uint32_t) , "max_stack_usage must be >= 4");

	void start(){
		mIsRunning = true;
	}

	void stop(){
		mIsRunning = false;
	}

	bool isRunning(){
		return mIsRunning;
	}

	uint16_t getStackUsage(){
		return mStackUsage;
	}

	uint16_t getMaxStackUsage(){
		return mMaxStackUsage;
	}

	template<typename T>
	void init(T *t) {
		mStackUsage = 0;
		mMaxStackUsage = 0;
		mIsRunning = false;
	}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 
	template<typename T>
	void makePreExe(T *t){

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
	template<typename T>
	void makePostExe(T *t){
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

	template<typename T>
	void makePreDel(T *t){}
	
private:

	static uint32_t *sSp;

	uint16_t mStackUsage;
	uint16_t mMaxStackUsage;
	bool mIsRunning;

	static const uint32_t kStackPattern = 0xAACCBBDD;
};



