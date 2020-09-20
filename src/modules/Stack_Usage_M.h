#pragma once

// debug tool : when started, this module will calculate the approximative
// stack usage of the process
// it shouldn't be always running as the cpu usage of this module can be high


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

	void init() {
		mStackUsage = 0;
		mMaxStackUsage = 0;
		mIsRunning = false;
	}

	bool isExeReady() const { return true; }

	bool isDelReady() const { return true; } 

	void makePreExe(){

		if(!mIsRunning){
			return;
		}
	
		const auto size=max_stack_usage/sizeof(uint32_t);
		uint32_t s[size];

		mSp = s;

		uint16_t i=0;
		while(i < size){
			s[i++] = kStackPattern;
		}

	}

	void makePostExe(){

		if(!mIsRunning){
			return;
		}	

		const auto size=max_stack_usage/sizeof(uint32_t);
		uint32_t s[size];

		if(mSp != s){
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

	void makePreDel(){}
	
private:

	uint32_t *mSp;

	uint16_t mStackUsage;
	uint16_t mMaxStackUsage;
	bool mIsRunning;

	static const uint32_t kStackPattern = 0xAACCBBDD;
};



