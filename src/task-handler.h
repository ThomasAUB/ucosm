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

#include <limits>
#include "IScheduler.h"



template<typename caller_t, typename task_module, size_t task_count>
class TaskHandler : public IScheduler
{

	using task_index_t = uint8_t;

	static_assert(task_count < std::numeric_limits<task_index_t>::max() , 
	              "Task count too high");	

	struct TaskItem : public task_module{
		constexpr TaskItem():index(sCounterIndex++){}
		const task_index_t index;
	private:
		static size_t sCounterIndex;
	};

    template<int N=0> 
	struct cpy_throw{ static_assert(N!=0, "Unauthorized copy"); };

	
	 
public:
    
	struct TaskHandle{
		void operator=(const TaskHandle&){cpy_throw t;};
		TaskItem* operator->(){return mP;}
		bool operator()(){return mP;}
		private:
		TaskItem* mP;
		friend class TaskHandler<caller_t, task_module, task_count>;
	};

	using task_function_t = void (caller_t::*)(TaskHandle);
	 
	TaskHandler(){}
	
	bool schedule() final {

		bool hasExe = false;
		
		task_index_t i=0;
		
		do{
		
			if( mFunctions[i] && mTasks[i].isExeReady() ){
				
				mTasks[i].makePreExe();
				TaskHandle h;
				h.mP = &mTasks[i];
				(static_cast<caller_t *>(this)->*mFunctions[i])(h);

				mTasks[i].makePostExe();
				
				hasExe = true;
			}
		
		}while(++i < task_count);
		
		return hasExe;
	}
		
	bool createTask(task_function_t inFunc, TaskHandle *ioHandle = nullptr){
				
		// allocation
		task_index_t i=0;
		
		do{
			if(!mFunctions[i]){
				
				mFunctions[i] = inFunc;
				
				if(ioHandle != nullptr){
					
					ioHandle->mP = &mTasks[i];
					
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

	bool deleteTask(TaskHandle inHandle){
		
		if(!inHandle.mP){ return false; }
		
		task_index_t i = inHandle.mP->index;

		if(mTasks[i].isDelReady()){
			
			mTasks[i].makePreDel();
			
			mFunctions[i] = nullptr;
			
			if(mHandlePtr[i]){ 
				// client's handle exists
				if(mHandlePtr[i]->mP == &mTasks[i]){ 
					// client's handle is valid
					mHandlePtr[i]->mP = nullptr; // reset client's handle
				}
				mHandlePtr[i] = nullptr;
			}
		}
	}

private:

	TaskItem mTasks[task_count];

	task_function_t mFunctions[task_count];
	
	TaskHandle *mHandlePtr[task_count];

};


template<typename Caller_t, typename task_traits, size_t task_count>
size_t TaskHandler<Caller_t, task_traits, task_count>::TaskItem::sCounterIndex = 0;

