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

#include "ucosm/utils/u_fifo.h"

// Allow to send data to a specific task

template<typename data_t, uint16_t fifo_size, bool auto_release = false>
struct Signal_M {

    template<typename inData_t, uint16_t inFifo_size, bool inAuto_release>
    bool sendSignal(Signal_M<inData_t, inFifo_size, inAuto_release> *inReceiver,
            data_t inData) {
        if (!inReceiver) {
            return false;
        }
        return (inReceiver->mRxData.push(inData));
    }

    data_t receiveSignal() {
        return mRxData.pop();
    }

    bool hasData() {
        return !mRxData.isEmpty();
    }

    void init() {
        mRxData.flush();
    }

    bool isExeReady() {
        return true;
    }

    bool isDelReady() {
        if (auto_release) {
            return true;
        } else {
            return mRxData.isEmpty();
        }
    }

    void makePreExe() {
    }

    void makePreDel() {
    }

    void makePostExe() {
    }

private:

    uFifo<data_t, fifo_size> mRxData;
};

