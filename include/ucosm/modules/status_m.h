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

// provides a simple status container

struct Status_M {

    bool isRunning() {
        return (mStatus & eRunning);
    }

    bool isStarted() {
        return (mStatus & eStarted);
    }

    bool isPending() {
        return !(mStatus & eStarted);
    }

    void setLock(bool inState) {
        if (inState) {
            if (mLocked < 0xFF) {
                mLocked++;
            }
        } else {
            if (mLocked > 0) {
                mLocked--;
            }
        }
    }

    void setSuspend(bool inState) {
        if (mLocked) {
            return;
        }
        if (inState) {
            if (mSuspended < 0xFF) {
                mSuspended++;
            }
        } else {
            if (mSuspended > 0) {
                mSuspended--;
            }
        }
    }

    void init() {
        mStatus = 0;
        mSuspended = 0;
        mLocked = 0;
        mExeCount = 0;
    }

    bool isExeReady() const {
        return (mSuspended == 0);
    }

    bool isDelReady() const {
        return (mLocked == 0);
    }

    void makePreExe() {
        mStatus |= eRunning;
    }

    void makePreDel() {
    }

    void makePostExe() {
        mStatus &= ~eRunning;
        mStatus |= eStarted;
        mExeCount++;
    }

private:

    enum eSystemStatus : uint8_t {
        eRunning = 0b00000001, eStarted = 0b00000010
    };

    uint8_t mStatus;
    uint8_t mLocked;
    uint8_t mSuspended;
    uint8_t mExeCount;

};

