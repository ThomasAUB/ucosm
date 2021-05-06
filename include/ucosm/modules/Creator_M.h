#pragma once

#include "utils/MemPool_32.h"

template<typename T, size_t ObjectCount>
struct Creator_M {

    template<typename ...args_t>
    T* create(args_t ...args) {
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

    static MemPool_32<ObjectCount, sizeof(T)> sMem;

    T *mP;

    bool mAutoRelease;
};

template<typename T, size_t ObjectCount>
MemPool_32<ObjectCount, sizeof(T)> Creator_M<T, ObjectCount>::sMem;

