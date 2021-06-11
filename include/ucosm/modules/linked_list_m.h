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

// automatically updated linked list of tasks by chronology of execution
template<uint32_t listIndex = 0>
struct LinkedList_M //: public ListItem // 9 bytes
{

    LinkedList_M<listIndex>* getNext() {
        return mNext;
    }
    LinkedList_M<listIndex>* getPrev() {
        return mPrev;
    }

    void init() {
        mPrev = mNext = nullptr;
        mIsStarted = false;
    }

    bool isExeReady() const {
        return true;
    }

    bool isDelReady() const {
        return true;
    }

    void makePreExe() {

        if (mIsStarted) {
            return;
        }

        mIsStarted = true;

        if (sTopHandle) {
            sTopHandle->mNext = this;
            mPrev = sTopHandle;
        }
        sTopHandle = this;
    }

    void makePreDel() {

        if (sTopHandle == this) {
            if (mPrev) {
                sTopHandle = mPrev;
            } else {
                sTopHandle = nullptr;
            }
        }
        if (mPrev && mNext) {
            mPrev->mNext = mNext;
            mNext->mPrev = mPrev;
        } else if (mPrev) {
            mPrev->mNext = nullptr;
        } else if (mNext) {
            mNext->mPrev = nullptr;
        }
    }

    void makePostExe() {
    }

private:

    static LinkedList_M<listIndex> *sTopHandle;

    LinkedList_M<listIndex> *mNext;
    LinkedList_M<listIndex> *mPrev;

    bool mIsStarted;

};

template<size_t listIndex>
LinkedList_M<listIndex> *LinkedList_M<listIndex>::sTopHandle = nullptr;

