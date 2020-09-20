/*
 * Copyright (C) 2020 Thomas AUBERT <aubert.thms@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Thomas AUBERT'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * uCosmDev IS PROVIDED BY Thomas AUBERT ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Thomas AUBERT OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#pragma once

#include <cstddef>

#include "IScheduler.h"

// sCnt
#include "modules/ucosm-sys-data.h"
uint8_t SysKernelData::sCnt = 0;


template<typename handler_t, size_t max_handler_count> 
class Kernel : public IScheduler
{

	using handler_index_t = uint8_t;
	 
public:

	Kernel() : mHandlerCount(0), mIdleTask(nullptr)
	{}

	template<typename T>
	bool addHandler(T *inHandler){
		if(mHandlerCount == max_handler_count){ return false; }
		mHandlers[mHandlerCount] = static_cast<IScheduler *>(inHandler);
		mHandlerTraits[mHandlerCount].init();		
		mHandlerCount++;
		return true;
	}
	
	handler_t *getHandle(IScheduler *inHandler){
		handler_index_t i;
		if(getHandlerIndex(inHandler, i)){
			return &mHandlerTraits[i];
		}
		return nullptr;
	}

	void removeHandler(IScheduler *inHandler){
		handler_index_t i;
		if(getHandlerIndex(inHandler, i)){

			if(!mHandlerTraits[i].isDelReady())
			{
				return;
			}

			mHandlerTraits[i].makePreDel();
			
			// shift handlers for contiguous array
			while(i<mHandlerCount-1)
			{
				mHandlers[i] = mHandlers[i+1];
				mHandlerTraits[i] = mHandlerTraits[i+1];
				i++;
			}
			mHandlerCount--;
		}
	}
	

	bool schedule () final {
	
		bool hasExe = false;	
				
		handler_index_t i = 0;
		
		SysKernelData::sCnt++;

		while(i < mHandlerCount)
		{
							
			if(mHandlers[i] && mHandlerTraits[i].isExeReady())
			{
				mHandlerTraits[i].makePreExe();
				hasExe |= mHandlers[i]->schedule();
				mHandlerTraits[i].makePostExe();
			}
			i++;
		}

		if(!hasExe) 
		{
			if(mIdleTask)
			{
				// idle task if exists
				mIdleTask();
			}
		}			

		return fullCycleExe;
	}

	void setIdleTask(void (*inIdleTask)())
 	{
		mIdleTask = inIdleTask;
	}


private:


	bool getHandlerIndex(IScheduler *inScheduler, handler_index_t& ioIndex)
 	{
		if(!mHandlerCount)
		{
			return false;
		}
		ioIndex = 0;	
		do
		{
			if(mHandlers[ioIndex] == inScheduler){
				return true;
			}
		}while(++ioIndex<mHandlerCount);
		return false;
	}

	IScheduler *mHandlers[max_handler_count];

	handler_t mHandlerTraits[max_handler_count];
	
	handler_index_t mHandlerCount;

	void (*mIdleTask)();

};


