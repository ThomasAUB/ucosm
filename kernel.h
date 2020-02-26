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

#include "task-handler.h"

#include "uscosm-sys-data.h"




template<typename handler_t, index_t max_handler_count> 
struct Kernel : public iScheduler
{

	Kernel() : mHandlerCount(0)
	{}

	handler_t *addHandler(iScheduler *inHandler)
	{
		if(mHandlerCount == max_handler_count){ return nullptr; }
		mHandlers[mHandlerCount] = inHandler;
		mHandlerTraits[mHandlerCount].init();		
		return &mHandlerTraits[mHandlerCount++];
	}

	bool removeHandle(handler_t *inHandler)
	{
		index_t i = inHandler->index;	
		while(i<mHandlerCount-1)
		{
			mHandlers[i] = mHandlers[i+1];
			mHandlerTraits[i] = mHandlerTraits[i+1];
			i++;
		}
		mHandlerCount--;
	}
	

	bool schedule(tick_t inMinDuration = 0)
	{
		
		bool hasExe = false;
		tick_t startTick = SysKernelData::sGetTick();
		
		do
		{
			
			index_t i = 0;
			SysKernelData::sCnt++;
			
			do
			{
				
				if(mHandlers[i] && mHandlerTraits[i].isExeReady())
				{
					mHandlerTraits[i].makePreExe();
					hasExe |= mHandlers[i]->schedule();
					mHandlerTraits[i].makePostExe();
				}
					
			}while(++i < mHandlerCount);

			/*
			if(!hasExe)
			{
				// idle task if exists
			}
			*/
			
		}while( ( SysKernelData::sGetTick() - startTick ) < inMinDuration );

		return hasExe;
	}


private:

	iScheduler *mHandlers[max_handler_count];

	handler_t mHandlerTraits[max_handler_count];
	
	index_t mHandlerCount;

};


