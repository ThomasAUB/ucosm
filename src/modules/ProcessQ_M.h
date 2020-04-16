#pragma once


// simple order process
struct ProcessQ_M
{

	void executeAfter(ProcessQ_M *inPrev){
		mPrev = inPrev;
	}

	void init(){
		mPrev = nullptr;
		mIsStarted = false;
	}

    bool isExeReady() const {
		if(mPrev){
			return mPrev->mIsStarted;
		}else{
			return true;
		}
	}

	bool isDelReady() const {return true;}

	void makePreExe(){}

	void makePreDel(){
		//  problem:
		// ''this'' still exists
		// so next still have this as prev
		// 
	}

	void makePostExe(){
		mIsStarted = true;	
	}

private:

	ProcessQ_M *mPrev;
	bool mIsStarted;

};




