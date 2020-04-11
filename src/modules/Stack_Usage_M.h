#pragma once


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
		volatile uint32_t s[size];

		sSp = s;

		uint16_t i=0;
		while(i < size){
			s[i] = kStackPattern;
		}

	}
	template<typename T>
	void makePostExe(T *t){

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
					mStackUsage = mMaxStackUsage;
				}
				break;
			}
			i++;
		}
	
		
	}

	template<typename T>
	void makePreDel(T *t){}
	
private:

	static uint32_t *sSp;

	uint16_t mStackUsage, mMaxStackUsage;
	bool mIsRunning;

	static const uint32_t kStackPattern = 0xAACCBBDD;
};

template<uint16_t max_stack_usage>
uint32_t *Stack_Usage_M<max_stack_usage>::sSp;

