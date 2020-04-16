#pragma once


// automatically updated linked list of tasks by chronology of execution
template<int listIndex = 0>
struct LinkedList_M //: public ListItem // 9 bytes
{

	LinkedList_M<listIndex> *getNext() { return mNext; }
	LinkedList_M<listIndex> *getPrev() { return mPrev; }

	void init(){   		
		mPrev = mNext = nullptr;
		mIsStarted = false;
	}

	bool isExeReady() const { return true; }

	bool isDelReady() const { return true; } 

	void makePreExe();

	void makePreDel();

	void makePostExe(){}

private:

	static LinkedList_M<listIndex> *sTopHandle;
	
 	LinkedList_M<listIndex> *mNext;
	LinkedList_M<listIndex> *mPrev;
	
	bool mIsStarted;
	
};




