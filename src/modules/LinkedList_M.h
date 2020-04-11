#pragma once

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
		//static_assert(std::is_base_of<Status, T>::value, "LinkedList must implement Status");
		//t->get<Status>().setStatus(0);
		mPrev = mNext = nullptr;
	}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 
	template<typename T>
	void makePreExe(T *t)
	{
		if(reinterpret_cast<Status_M *>(this)->isStarted()){return;}

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

template<int listIndex>
ListItem *LinkedList_M<listIndex>::sTopHandle = nullptr;
