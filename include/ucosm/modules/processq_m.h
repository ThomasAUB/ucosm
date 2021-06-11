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

// allows to create process execution queues,
// as you can chose to single or double link process,
// you can  elaborate complex execution start patterns

struct ProcessQ_M {

    void setFirst() {
        // parse previous
        if (!mPrev) {
            return;
        }

        mPrev->mNext = mNext;
        mNext->mPrev = mPrev;

        ProcessQ_M *t = mPrev;

        while (t->mPrev) {
            t = t->mPrev;
        }

        mNext = t;
        t->mPrev = this;

        mPrev = nullptr;
    }

    void setLast() {
        // parse nexts
        if (!mNext) {
            return;
        }

        mNext->mPrev = mPrev;
        mPrev->mNext = mNext;

        ProcessQ_M *t = mNext;

        while (t->mNext) {
            t = t->mNext;
        }

        mPrev = t;
        t->mNext = this;

        mNext = nullptr;
    }

    void executeBefore(ProcessQ_M *inNext) {
        if (!inNext) {
            return;
        }
        mNext = inNext;
        mNext->mPrev = this;
    }

    void executeAfter(ProcessQ_M *inPrev) {
        if (!inPrev) {
            return;
        }
        mPrev = inPrev;
        mPrev->mNext = this;
    }

    void init() {
        mPrev = nullptr;
        mNext = nullptr;
    }

    bool isExeReady() const {
        return (mPrev == nullptr);
    }

    bool isDelReady() const {
        return true;
    }

    void makePreExe() {
    }

    void makePreDel() {
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
        if (mNext) {
            mNext->mPrev = nullptr;
        }
    }

private:

    ProcessQ_M *mPrev;
    ProcessQ_M *mNext;

};

