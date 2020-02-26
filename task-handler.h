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


#include "traits.h"




template<typename Caller_t, typename task_traits, index_t task_count, uint16_t max_param_size = 0>
struct TaskHandler : public iScheduler
{
	
	static_assert(task_count < max_index-1 , "Task count too high");
	
	using task_function_t = void (Caller_t::*)(void *);

	using TaskHandle = task_traits*;

	
	bool schedule(tick_t t)
	{

		bool hasExe = false;
		
		index_t i=0;
		
		do{
			if(mFunctions[i] && mTasks[i].isExeReady())
			{
				mCurrHandleIndex = i;
				mTasks[i].makePreExe();
				(static_cast<Caller_t *>(this)->*mFunctions[i])(params[i]);
				mTasks[i].makePostExe();
				mCurrHandleIndex = max_index;
				hasExe = true;
			}
		}while(++i < task_count);
		
		return hasExe;
	}
	
	TaskHandle thisTaskHandle()
	{
		if(mCurrHandleIndex == max_index)
		{
			catchException("thisTask() not allowed in this context");
			return 0;
		}
		return &mTasks[mCurrHandleIndex];
	}

	template<typename T>
	bool createTask(task_function_t inFunc, T *p, TaskHandle *ioHandle = nullptr)
	{
		static_assert(sizeof(T) <= max_param_size, "Param size too large");
		return createTask(inFunc, p, sizeof(T), ioHandle);
	}

	
	
	bool createTask(task_function_t inFunc, void *p = nullptr, uint16_t inParamSize = 0, TaskHandle *ioHandle = nullptr)
	{					
		if(inParamSize > max_param_size)
		{
			catchException("Param size too large");
			return false; 
		}
		
		// allocation
		index_t i=0;
		do
		{
			if(!mFunctions[i])
			{
				copyParams(i, p, inParamSize);
				mFunctions[i] = inFunc;
				if(ioHandle != nullptr)
				{
					*ioHandle = &mTasks[i];
					mHandlePtr[i] = ioHandle;
				}else{
					mHandlePtr[i] = nullptr;
				}
				mTasks[i].init();
				return true;
			}
		}while(++i < task_count);

		return false;
	}

	bool deleteTask(TaskHandle inHandle)
	{
		if(!inHandle){ return false; }
		
		index_t i = inHandle->index;

		if(mTasks[i].isDelReady())
		{
			
			mTasks[i].makePreDel();
			mFunctions[i] = nullptr;
			
			if(mHandlePtr[i] && *mHandlePtr[i]==&mTasks[i])
			{
				*mHandlePtr[i] = nullptr;
			}
			
			mHandlePtr[i] = nullptr;
		}
	}
 
private:

	bool copyParams(index_t i, void *p, uint16_t inParamSize){

        if(inParamSize > max_param_size || !inParamSize || p == nullptr){
            return false;
        }
         // copy parameters if any
        uint8_t *pSrc   = (uint8_t *)p;
        uint8_t *pDest  = (uint8_t *)params[i];
        uint8_t *pEnd   = (uint8_t *)(pSrc+inParamSize);
        do{
            *pDest++ = *pSrc++;
        }while(pSrc != pEnd);
        
        return true;
    }

	virtual void catchException(const char *inErrMsg){}
	
	

	task_function_t mFunctions[task_count]; // 4 bytes

	TaskHandle *mHandlePtr[task_count]; // 4 bytes

	task_traits mTasks[task_count]; // 1 byte + user defined

	// best as index : can't be modified when passed
	index_t mCurrHandleIndex;// 1 byte
	
	uint8_t params[task_count][max_param_size]; // user defined

	
};

// minimum 10 bytes
