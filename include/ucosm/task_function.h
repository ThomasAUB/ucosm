/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2020 Thomas AUBERT                                                *
 *                                                                                 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy    *
 * of this software and associated documentation files (the "Software"), to deal   *
 * in the Software without restriction, including without limitation the rights    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is           *
 * furnished to do so, subject to the following conditions:                        *
 *                                                                                 *
 * The above copyright notice and this permission notice shall be included in all  *
 * copies or substantial portions of the Software.                                 *
 *                                                                                 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 * SOFTWARE.                                                                       *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include <limits>
#include "itask.h"
#include "void_m.h"

#include "stdint.h"

// this class uses CRTP to call the task functions
// class MyClass : TaskFunction<MyClass, 2>{ /*...*/ };

template<typename caller_t, uint32_t task_count, typename module_M = void_M>
class TaskFunction: public ITask {

    using task_index_t = uint8_t;

    static const task_index_t kInvalidIdx =
            std::numeric_limits<task_index_t>::max();

    static_assert(task_count < kInvalidIdx, "Task count too high");

    static_assert(task_count > 0, "Task count must be at least 1");

    struct TaskItem: public module_M {
        constexpr TaskItem() :
                index(sCounterIndex++) {
        }
        const task_index_t index;
    private:
        static task_index_t sCounterIndex;
    };

    enum throwExcept {
        eIllegalCopy, eExceptCount
    };

    template<int N>
    struct throw_except {
        static_assert(N!=eIllegalCopy, "Illegal copy");
    };

public:

    struct TaskHandle {

        TaskHandle() :
                mP(nullptr) {
        }
        ~TaskHandle() {
            mP = nullptr;
        }

        using task_function_t = void (caller_t::*)(TaskHandle);

        void operator =(const TaskHandle&) {
            throw_except<eIllegalCopy> _;
        }

        TaskItem* operator ->() {
            return mP;
        }
        TaskItem* operator ()() {
            return mP;
        }
        bool operator ==(task_function_t f) {
            if (!mP) {
                return false;
            }
            return (handler->getTaskFunction(*this) == f);
        }

    private:
        TaskItem *mP;
        static TaskFunction<caller_t, task_count, module_M> *handler;
        friend class TaskFunction<caller_t, task_count, module_M> ;
    };

    TaskFunction() :
            mFunctions { nullptr }, mHandlePtr { nullptr }, mActiveTaskCount(0) {
        TaskHandle::handler = this;
    }

    using task_function_t = typename TaskHandle::task_function_t;

    // creates a child task to schedule
    // returns true if success, false otherwise
    // a function can be executed through several tasks
    // ioHandle is optional, it is used to access module after creation
    // if the task associated with a TaskHandle exists : myTaskHandle() == true
    // if the TaskHandle still exists at task deletion, it will be reset : myTaskHandle() == false
    // even if the task has been deleted with the TaskHandle passed to the function
    bool createTask(task_function_t inFunc, TaskHandle *ioHandle = nullptr);

    // deletes a child task
    // returns true is success, false otherwise
    bool deleteTask(TaskHandle &inHandle);

    // resturns the function pointer of the task
    task_function_t getTaskFunction(TaskHandle inHandle);

    // returns the number of currently active tasks
    task_index_t getTaskCount();

    // to call periodically
    // manually or through another instance
    // returns true if a task has been executed,
    // false otherwise
    bool schedule() final;

protected:

    // allocate task on the specified slot
    // returns true if success(slot was free), false otherwise
    bool createTaskAt(task_function_t inFunc, task_index_t i,
            TaskHandle *ioHandle = nullptr);

private:

    void allocate(task_function_t inFunc, task_index_t i, TaskHandle *ioHandle);

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

template<typename caller_t, uint32_t task_count, typename module_M>
typename TaskFunction<caller_t, task_count, module_M>::task_index_t TaskFunction<
        caller_t, task_count, module_M>::TaskItem::sCounterIndex = 0;

template<typename caller_t, uint32_t task_count, typename module_M>
TaskFunction<caller_t, task_count, module_M> *TaskFunction<caller_t, task_count,
        module_M>::TaskHandle::handler = nullptr;

template<typename caller_t, uint32_t task_count, typename module_M>
bool TaskFunction<caller_t, task_count, module_M>::schedule() {

    static task_index_t sI = kInvalidIdx;

    if (sI != kInvalidIdx) {
        // error : task handler is already scheduling
        return false;
    }

    sI = 0;

    bool hasExe = false;

    task_index_t taskCounter = 0;

    while (taskCounter < mActiveTaskCount && sI < task_count) {

        if (mFunctions[sI]) {

            taskCounter++;

            if (mTasks[sI].isExeReady()) {

                // build task handle
                mCurTask.mP = &mTasks[sI];

                mTasks[sI].makePreExe();

                // call task
                (static_cast<caller_t*>(this)->*mFunctions[sI])(mCurTask);

                // call "post exe" if the task
                // still exists
                if (mCurTask.mP) {
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

template<typename caller_t, uint32_t task_count, typename module_M>
bool TaskFunction<caller_t, task_count, module_M>::createTask(
        task_function_t inFunc, TaskHandle *ioHandle) {

    // allocation
    task_index_t i = 0;
    do {

        // check if slot is free
        if (!mFunctions[i]) {
            allocate(inFunc, i, ioHandle);
            return true;
        }

    } while (++i < task_count);

    return false;
}

template<typename caller_t, uint32_t task_count, typename module_M>
bool TaskFunction<caller_t, task_count, module_M>::deleteTask(
        TaskHandle &inHandle) {

    // check if handle is initialize
    if (!inHandle()) {
        return false;
    }

    task_index_t i = inHandle.mP->index;

    // check if the task is ready to be deleted
    if (mTasks[i].isDelReady()) {

        // destroy task
        mTasks[i].makePreDel();

        // reset task function pointer
        mFunctions[i] = nullptr;

        // deleting task is the one currently scheduled
        if (inHandle.mP == mCurTask.mP) {
            mCurTask.mP = nullptr;
        }

        // delete usr's handle
        inHandle.mP = nullptr;

        // check if client's handle exists or existed
        if (mHandlePtr[i]) {

            // client's handle is valid
            if (mHandlePtr[i]->mP == &mTasks[i]) {

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
template<typename caller_t, uint32_t task_count, typename module_M>
typename TaskFunction<caller_t, task_count, module_M>::task_function_t
TaskFunction<caller_t, task_count, module_M>::getTaskFunction(TaskHandle inHandle) {
    if (!inHandle()) {
        return 0;
    }
    return mFunctions[inHandle.mP->index];
}

// returns the count of currently active tasks
template<typename caller_t, uint32_t task_count, typename module_M>
typename TaskFunction<caller_t, task_count, module_M>::task_index_t
TaskFunction<caller_t, task_count, module_M>::getTaskCount() {
    return mActiveTaskCount;
}

template<typename caller_t, uint32_t task_count, typename module_M>
bool TaskFunction<caller_t, task_count, module_M>::createTaskAt(
        task_function_t inFunc, task_index_t i, TaskHandle *ioHandle) {
    // check if function pointer is free
    if (mFunctions[i]) {
        return false;
    }
    allocate(inFunc, i, ioHandle);
    return true;
}

template<typename caller_t, uint32_t task_count, typename module_M>
void TaskFunction<caller_t, task_count, module_M>::allocate(
        task_function_t inFunc, task_index_t i, TaskHandle *ioHandle) {

    // set task function pointer
    mFunctions[i] = inFunc;

    // check if a handle has been passed
    if (ioHandle != nullptr) {
        // set module pointer
        ioHandle->mP = &mTasks[i];
        // set handle pointer
        mHandlePtr[i] = ioHandle;
    } else {
        // reset handle pointer
        mHandlePtr[i] = nullptr;
    }
    mTasks[i].init();
    mActiveTaskCount++;
}
