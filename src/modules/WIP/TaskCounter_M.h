#pragma once


struct Counter_M
{

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

size_t Counter_M::mActiveCount = 0;
