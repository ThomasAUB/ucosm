#pragma once

// automatically updated linked list of tasks by chronology of execution
template<size_t listIndex = 0>
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

