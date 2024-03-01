#pragma once

#include "itask.hpp"

namespace ucosm {

    struct Scheduler : ITask {

        Scheduler(std::string_view inName = "") :
            ITask(inName) {}

        void run() override {

            if (mTasks.empty() || mTasks.front().getRank() > 0) {
                // no tasks or not ready
                return;
            }

            while (!mTasks.empty() && mTasks.front().getRank() <= 0) {
                mTasks.front().run();
            }
        }

        rank_t getRank() const override {
            if (mTasks.empty()) {
                return 1;
            }
            else {
                return mTasks.front().getRank();
            }
        }

        bool addTask(ITask& inTask) {
            if (!inTask.init()) {
                return false;
            }
            mTasks.push_front(inTask);
            return true;
        }

    private:
        ulink::List<ITask> mTasks;
    };

}