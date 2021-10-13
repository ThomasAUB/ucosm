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

#include "stdint.h"

template<uint8_t block_count, uint32_t block_size, uint8_t alignment = 4>
struct MemPool {

    static_assert(block_count <= 32, "block count can't exceed 32");

    static_assert((block_size % alignment) == 0, "block_size must be a multiple of alignment");

    MemPool() : mMap(0) {}

    template<typename T, typename ... args_t>
    bool allocate(T **p, args_t&&... args);

    template<typename T>
    bool release(T **p);

    uint8_t getSizeLeft();

private:

    alignas(alignment) uint8_t mBlocks[block_count][block_size];
    uint32_t mMap;

};


template<uint8_t block_count, uint32_t block_size, uint8_t alignment>
template<typename T, typename ... args_t>
bool MemPool<block_count, block_size, alignment>::allocate(T **p, args_t&&... args) {

    static_assert(sizeof(T) <= block_size, "Object is bigger than block size");

    // check space left
    if (mMap == (1 << block_count) - 1) {
        return false;
    }

    uint8_t i = 0;

    do {
        // check if slot is free
        if (!(mMap & (1 << i))) {

            // take slot
            mMap |= (1 << i);

            // allocate
            *p = new (mBlocks[i]) T(args...);

            return (*p != nullptr);
        }

    } while (++i < block_count);

    // couldn't allocate
    return false;
}


template<uint8_t block_count, uint32_t block_size, uint8_t alignment>
template<typename T>
bool MemPool<block_count, block_size, alignment>::release(T **p) {

    if (*p == nullptr) {
        return false;
    }

    uint8_t i = 0;

    void *ip = reinterpret_cast<void*>(*p);

    do {

        // search for the matching address
        if (mBlocks[i] == ip) {

            // explicitly call destructor
            (*p)->T::~T();

            // release slot
            mMap &= ~(1 << i);

            *p = nullptr;

            return true;
        }

    } while (++i < block_count);

    return false;
}


template<uint8_t block_count, uint32_t block_size, uint8_t alignment>
uint8_t MemPool<block_count, block_size, alignment>::getSizeLeft() {
    uint8_t sizeLeft = 0;
    for (uint8_t i = 0; i < block_count; i++) {
        if (!(mMap & (1 << i))) {
            sizeLeft++;
        }
    }
    return sizeLeft;
}
