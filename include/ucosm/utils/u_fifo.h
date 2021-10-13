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

template<typename T, size_t Size>
struct uFifo {

    uFifo() :
            mOldV(0), mNewV(0), mIsEmpty(true), mIsFull(false) {
    }

    bool push(T data) {
        if (mIsFull) {
            return false;
        }
        size_t n = (mNewV + 1) % Size;
        if (n == mOldV) {
            mIsFull = true;
        }
        mElems[mNewV] = data;
        mNewV = n;
        mIsEmpty = false;
        return true;
    }

    // removes oldest item
    T pop() {
        if (mIsEmpty) {
            return T();
        }
        size_t n = mOldV;
        mOldV = (mOldV + 1) % Size;
        if (mOldV == mNewV) {
            mIsEmpty = true;
        }
        mIsFull = false;
        return mElems[n];
    }

    // removes newest item
    T popBack() {
        if (mIsEmpty) {
            return T();
        }

        mNewV = (mNewV + Size - 1) % Size;

        if (mOldV == mNewV) {
            mIsEmpty = true;
        }

        mIsFull = false;

        return mElems[mNewV];
    }

    bool isEmpty() {
        return mIsEmpty;
    }

    bool isFull() {
        return mIsFull;
    }

    void flush() {
        mOldV = 0;
        mNewV = 0;
        mIsEmpty = true;
        mIsFull = false;
    }

    T getLast() {
        return mElems[(mNewV + Size - 1) % Size];
    }

    size_t getSize() {
        if (mOldV > mNewV) {
            return mOldV - mNewV;
        } else {
            return mNewV - mOldV;
        }
    }

private:

    size_t mOldV, mNewV;
    bool mIsEmpty :1;
    bool mIsFull :1;
    T mElems[Size];

};

