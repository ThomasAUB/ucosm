#pragma once

#include "ModuleKit_M.h"
#include "Status_M.h"

struct ListItem
{
	ListItem* mPrev;
	ListItem* mNext;
};



// automatically updated linked list of tasks by chronology of execution
template<int listIndex>
struct LinkedList_M : public ListItem // 9 bytes
{

	ListItem *getNext() { return mNext; }
	ListItem *getPrev() { return mPrev; }

	void init(){   		
		mPrev = mNext = nullptr;
		mIsStarted = false;
	}

	bool isExeReady() const { return true; }

	bool isDelReady() const { return true; } 

	void makePreExe(){

		if(mIsStarted){ return; }

		mIsStarted = true;
		
		if(sTopHandle)
		{
			sTopHandle->mNext = this;
			mPrev = sTopHandle;
		}
		sTopHandle = this;
	}

	void makePreDel(){
		
		if(sTopHandle == this)
		{
			if(mPrev){
				sTopHandle = mPrev;
			}else{
				sTopHandle = nullptr;
			}
		}
		if(mPrev && mNext)
		{
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

	static ListItem *sTopHandle;
	bool mIsStarted;
	
};

template<int listIndex>
ListItem *LinkedList_M<listIndex>::sTopHandle = nullptr;


