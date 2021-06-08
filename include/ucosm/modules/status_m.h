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

