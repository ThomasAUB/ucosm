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

    TaskHandler() : 
    mFunctions{ nullptr }, mHandlePtr{ nullptr }, mActiveTaskCount(0)
    { TaskHandle::handler = this; }

    using task_function_t = typename TaskHandle::task_function_t;

    bool schedule() final {

    	static task_index_t sI = kInvalidIdx;

    	if(sI != kInvalidIdx){
            // error : task handler is already scheduling
    		return false;
    	}

    	sI = 0;

        bool hasExe = false;

        task_index_t taskCounter = 0;

        while(taskCounter < mActiveTaskCount && sI < task_count){

            if( mFunctions[sI] ){

            	taskCounter++;

            	if( mTasks[sI].isExeReady() ){

            		// build task handle
            		mCurTask.mP = &mTasks[sI];

					mTasks[sI].makePreExe();
					
                    // call task
					(static_cast<caller_t *>(this)->*mFunctions[sI])(mCurTask);

					// call "post exe" if the task
					// still exists
					if(mCurTask.mP){
						mTasks[sI].makePostExe();
					}

					hasExe = true;
            	}
            }
            sI++;
        }

        mCurTask.mP = nullptr;
		
        sI = kInvalidIdx;

        return hasExe;
    }
		
    bool createTask(task_function_t inFunc, TaskHandle *ioHandle = nullptr){
				
        // allocation
        task_index_t i=0;
        do{

            // check if slot is free
        	if(!mFunctions[i]){
        	   allocate(inFunc, i, ioHandle);
        	   return true;
        	}
		
        }while(++i < task_count);

        return false;
    }

    bool deleteTask(TaskHandle& inHandle){
		
        // check if handle is initialize
        if(!inHandle()){ return false; }
		
        task_index_t i = inHandle.mP->index;

        // check if the task is ready to be deleted
        if(mTasks[i].isDelReady()){
			
            // destroy task
            mTasks[i].makePreDel();
			
            // reset task function pointer
            mFunctions[i] = nullptr;

            // deleting task is the one currently scheduled
            if(inHandle.mP == mCurTask.mP){
            	mCurTask.mP = nullptr;
            }

            // delete usr's handle
            inHandle.mP = nullptr;

            // check if client's handle exists or existed
            if(mHandlePtr[i]){
                
                // client's handle is valid
                if(mHandlePtr[i]->mP == &mTasks[i]){

                    // reset client's handle
                    mHandlePtr[i]->mP = nullptr;

                }

                // reset task handle pointer
                mHandlePtr[i] = nullptr;
            }

            mActiveTaskCount--;

            return true;
        }

        return false;
    }

    // resturns the function pointer of the task
    task_function_t getTaskFunction(TaskHandle inHandle){
        if(!inHandle()){ return 0; }
        return mFunctions[inHandle.mP->index];
    }

    // returns the count of currently active tasks
    task_index_t getTaskCount(){
    	return mActiveTaskCount;
    }

protected:

    bool createTaskAt(task_function_t inFunc, task_index_t i, TaskHandle *ioHandle = nullptr){
        // check if function pointer is free
		if(mFunctions[i]){
			return false;
		}
		allocate(inFunc, i, ioHandle);
		return true;
	}
	
private:

    void allocate(task_function_t inFunc, task_index_t i, TaskHandle *ioHandle){

        // set task function pointer
		mFunctions[i] = inFunc;

        // check if a handle has been passed
		if(ioHandle != nullptr){
            // set module pointer
			ioHandle->mP = &mTasks[i];
            // set handle pointer
			mHandlePtr[i] = ioHandle;
		}else{
            // reset handle pointer
			mHandlePtr[i] = nullptr;
		}
		mTasks[i].init();
		mActiveTaskCount++;
    }
 
    // task's module(s) and index
    TaskItem mTasks[task_count];

    // task function pointers
    task_function_t mFunctions[task_count];
	
    // task handle pointers
    TaskHandle *mHandlePtr[task_count];

    // count of currently active tasks
    task_index_t mActiveTaskCount;

    // currently scheduled task
    TaskHandle mCurTask;
};


template<typename caller_t, size_t task_count, typename module_M>
size_t TaskHandler<caller_t, task_count, module_M>::TaskItem::sCounterIndex = 0;

template<typename caller_t, size_t task_count, typename module_M>
TaskHandler<caller_t, task_count, module_M>* TaskHandler<caller_t, task_count, module_M>::TaskHandle::handler = nullptr;

