#include "ProcessCounter_M.h"


size_t ProcessCounter_M::mActiveCount = 0;


void ProcessCounter_M::init(){
	mActiveCount++;
}

void ProcessCounter_M::makePreDel(){
	mActiveCount--;	
}

size_t ProcessCounter_M::getCount(){
	return mActiveCount;
}


