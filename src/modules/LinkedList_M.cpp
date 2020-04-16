#include "LinkedList_M.h"


template<int listIndex>
LinkedList_M<listIndex> *LinkedList_M<listIndex>::sTopHandle = nullptr;


	
template<int listIndex = 0>
void LinkedList_M<listIndex>::makePreExe(){

	if(mIsStarted){ return; }

	mIsStarted = true;
	
	if(sTopHandle)
	{
		sTopHandle->mNext = this;
		mPrev = sTopHandle;
	}
	sTopHandle = this;
}



template<int listIndex = 0>
void LinkedList_M<listIndex>::makePreDel(){
	
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



