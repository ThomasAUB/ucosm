#pragma once

#include "utils/u_fifo.h"

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

