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

#include "task_object.h"

#include "modules/utils/mem_pool.h"

template<uint32_t task_count, uint32_t max_task_size, typename module_M = void_M, uint8_t alignment = 4>
struct TaskObjectAllocator: TaskObject<task_count, module_M> {

    template<typename task_t, typename ... args_t>
    bool createTask(task_t **t, args_t &&... args);

    template<typename task_t>
    bool deleteTask(task_t **t);

private:

    MemPool<task_count, max_task_size + max_task_size % alignment, alignment> mPool;

};


template<uint32_t task_count, uint32_t max_task_size, typename module_M, uint8_t alignment>
template<typename task_t, typename ... args_t>
bool TaskObjectAllocator<task_count, max_task_size, module_M, alignment>::createTask(
        task_t **t, args_t &&... args) {

    // check if taskObject has enough room
    if (this->getTaskCount() == task_count) {
        return false;
    }

    // try to allocate task
    if (!mPool.allocate(t, args...)) {
        return false;
    }

    // add task to TaskObject
    return this->addTask(*t);
}

template<uint32_t task_count, uint32_t max_task_size, typename module_M, uint8_t alignment>
template<typename task_t>
bool TaskObjectAllocator<task_count, max_task_size, module_M, alignment>::deleteTask(task_t **t) {

    if(!this->removeTask(*t)) {
        return false;
    }

    return mPool.release(t);
}
