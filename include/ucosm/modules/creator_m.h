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

#include "../utils/mem_pool.h"

template<typename T, uint32_t object_count, uint8_t alignment = 4>
struct Creator_M {

    template<typename ... args_t>
    T* create(args_t&&...args) {
        sMem.allocate(&mP, args...);
        return mP;
    }

    bool destroy() {
        return sMem.release(&mP);
    }

    T* get() {
        return mP;
    }

    void setAutoRelease(bool inState) {
        mAutoRelease = inState;
    }

    bool isAutoRelease() {
        return mAutoRelease;
    }

    void init() {
        mP = nullptr;
        mAutoRelease = true;
    }

    bool isExeReady() const {
        return true;
    }

    bool isDelReady() {
        return (mAutoRelease || (mP == nullptr));
    }

    void makePreExe() {
    }

    void makePreDel() {
        destroy();
    }

    void makePostExe() {
    }

private:

    static MemPool<object_count, sizeof(T) + sizeof(T) % alignment, alignment> sMem;

    T *mP;

    bool mAutoRelease;
};

template<typename T, uint32_t object_count, uint8_t alignment>
MemPool<object_count, sizeof(T) + sizeof(T) % alignment, alignment> Creator_M<T, object_count, alignment>::sMem;

