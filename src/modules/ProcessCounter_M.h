#pragma once


struct ProcessCounter_M
{

	size_t getCount(){
		return mActiveCount;
	}

	void init(){
		mActiveCount++;
	}

    bool isExeReady() const {return true;}

	bool isDelReady() const {return true;}

	void makePreExe(){}

	void makePreDel(){
		mActiveCount--;	
	}

	void makePostExe(){}

private:

	static size_t mActiveCount;
};

