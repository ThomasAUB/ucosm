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

