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

#include "void_M.h"

template<typename caller_t, size_t task_count, typename module_M = void_M>
class TaskHandler : public IScheduler
{

    using task_index_t = uint8_t;

    static const task_index_t kInvalidIdx = std::numeric_limits<task_index_t>::max();

    static_assert(task_count < kInvalidIdx ,
    "Task count too high");	

    static_assert(task_count > 0 ,
    "Invalid task count");

    struct TaskItem : public module_M{
        constexpr TaskItem():index(sCounterIndex++){}
        const task_index_t index;
    private:
        static size_t sCounterIndex;
    };

    enum throwExcept{
        eIllegalCopy,
        eExceptCount
    };

    template<int N> 
    struct throw_except{ static_assert(N!=eIllegalCopy, "Illegal copy"); };

public:


    struct TaskHandle{

        TaskHandle ():mP(nullptr){}
        ~TaskHandle(){ mP=nullptr; }

        using task_function_t = void (caller_t::*)(TaskHandle);

		void 		operator =	(const TaskHandle&){throw_except<eIllegalCopy> t;};
		TaskItem* 	operator ->	(){return mP;}
		TaskItem*	operator ()	(){return mP;}
		bool 		operator ==	(task_function_t f){
			if(!mP){return false;}
			return (handler->getTaskFunction(*this) == f);
		}

    private:
        TaskItem* mP;
        static TaskHandler<caller_t, task_count, module_M> *handler;
        friend class TaskHandler<caller_t, task_count, module_M>;
    };
	
    TaskHandler() : mActiveTaskCount(0) { TaskHandle::handler = this; }

    using task_function_t = typename TaskHandle::task_function_t;

    bool schedule() final {

    	static task_index_t sI = kInvalidIdx;

    	if(sI != kInvalidIdx){
    		return false;
    	}

    	sI = 0;

        bool hasExe = false;

        task_index_t taskCounter = 0;

        while(sI < task_count && taskCounter < mActiveTaskCount){

            if( mFunctions[sI] ){

            	taskCounter++;

            	if( mTasks[sI].isExeReady() ){

					mTasks[sI].makePreExe();
					TaskHandle h;
					h.mP = &mTasks[sI];
					(static_cast<caller_t *>(this)->*mFunctions[sI])(h);

					mTasks[sI].makePostExe();

					hasExe = true;
            	}
            }
            sI++;
        }
		
        sI = kInvalidIdx;

        return hasExe;
    }
		
    bool createTask(task_function_t inFunc, TaskHandle *ioHandle = nullptr){
				
        // allocation
        task_index_t i=0;
        do{

        	if(!mFunctions[i]){
        	   allocate(inFunc, i, ioHandle);
        	   return true;
        	}
		
        }while(++i < task_count);

        return false;
    }

    bool deleteTask(TaskHandle inHandle){
		
        if(!inHandle()){ return false; }
		
        task_index_t i = inHandle.mP->index;

        if(mTasks[i].isDelReady()){
			
            mTasks[i].makePreDel();
			
            mFunctions[i] = nullptr;
			
            if(mHandlePtr[i]){
                // client's handle exists or existed
                if(mHandlePtr[i]->mP == &mTasks[i]){
                    // client's handle is valid
                    mHandlePtr[i]->mP = nullptr; // reset client's handle
                }
                mHandlePtr[i] = nullptr;
            }
            mActiveTaskCount--;
            return true;
        }
        return false;
    }

    task_function_t getTaskFunction(TaskHandle inHandle){
        if(!inHandle()){ return 0; }
        return mFunctions[inHandle.mP->index];
    }

    task_index_t getTaskCount(){
    	return mActiveTaskCount;
    }

protected:

    bool createTaskAt(task_function_t inFunc, task_index_t i, TaskHandle *ioHandle = nullptr){
		if(mFunctions[i]){
			return false;
		}
		allocate(inFunc, i, ioHandle);
		return true;
	}
	
private:

    void allocate(task_function_t inFunc, task_index_t i, TaskHandle *ioHandle){

		mFunctions[i] = inFunc;

		if(ioHandle != nullptr){
			ioHandle->mP = &mTasks[i];
			mHandlePtr[i] = ioHandle;
		}else{
			mHandlePtr[i] = nullptr;
		}
		mTasks[i].init();
		mActiveTaskCount++;
    }
 
    TaskItem mTasks[task_count];

    task_function_t mFunctions[task_count];
	
    TaskHandle *mHandlePtr[task_count];

    task_index_t mActiveTaskCount;
};


template<typename caller_t, size_t task_count, typename module_M>
size_t TaskHandler<caller_t, task_count, module_M>::TaskItem::sCounterIndex = 0;

template<typename caller_t, size_t task_count, typename module_M>
TaskHandler<caller_t, task_count, module_M>* TaskHandler<caller_t, task_count, module_M>::TaskHandle::handler = nullptr;

