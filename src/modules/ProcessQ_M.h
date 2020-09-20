#pragma once

// allows to create process execution queues,
// as you can chose to single or double link process,
// you can  elaborate complex execution start patterns


struct ProcessQ_M
{

	void setFirst(){
		// parse previous
		if(!mPrev){
			return;
		}

		mPrev->mNext = mNext;
		mNext->mPrev = mPrev;

		ProcessQ_M *t = mPrev;

		while(t->mPrev){
			t = t->mPrev;
		}

		mNext = t;
		t->mPrev = this;

		mPrev = nullptr;
	}

	void setlast(){
		// parse nexts
		if(!mNext){
			return;
		}

		mNext->mPrev = mPrev;
		mPrev->mNext = mNext;

		ProcessQ_M *t = mNext;

		while(t->mNext){
			t = t->mNext;
		}

		mPrev = t;
		t->mNext = this;

		mNext = nullptr;
	}

	void executeBefore(ProcessQ_M *inNext, bool doubleLink = true){
		if(!inNext){
			return;
		}

		mNext = inNext;

		if(doubleLink){
			inNext->mPrev = this;
		}
	}

	void executeAfert(ProcessQ_M *inPrev, bool doubleLink = true){
		if(!inPrev){
			return;
		}

		mPrev = inPrev;

		if(doubleLink){
			inPrev->mNext = this;
		}
	}

	void setState(bool inState){
		mState = inState;
	}

	void init(){
		mPrev = nullptr;
		mNext = nullptr;
		mState = false;
	}

    bool isExeReady() const {
		if(mPrev){
			return mPrev->mState;
		}else{
			return true;
		}
	}

	bool isDelReady() const {return true;}

	void makePreExe(){
		mState = true; // should we let user only set the state?
	}

	void makePreDel(){
		if(mPrev && mNext){
			mPrev->mNext = mNext;
			mNext->mPrev = mPrev;
		}else if(mPrev){
			mPrev->mNext = nullptr;
		}else if(mNext){
			mNext->mPrev = nullptr;
		}
	}

	void makePostExe(){}

private:

	ProcessQ_M *mPrev;
	ProcessQ_M *mNext;

	bool mState;

};




