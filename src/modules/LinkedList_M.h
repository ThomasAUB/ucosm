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

	template<typename T>
	void init(T *t)
	{   		
		mPrev = mNext = nullptr;
	}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 
	template<typename T>
	void makePreExe(T *t)
	{

		Status_M *st;
		//st = t->get<Status_M>();
		//if(t->get<Status_M>()->isStarted()){return;}

	
		if(sTopHandle)
		{
			sTopHandle->mNext = this;
			mPrev = sTopHandle;
		}
		sTopHandle = this;
	}
	template<typename T>
	void makePreDel(T *t)
	{
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
	template<typename T>
	void makePostExe(T *t){}

private:

	static ListItem *sTopHandle;
	
};


