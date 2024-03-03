#pragma once

#include "itask.hpp"
#include <limits>


namespace ucosm {

    template<typename rank_t>
    struct IScheduler : ITask<rank_t> {

        IScheduler(std::string_view inName = "") :
            ITask<rank_t>(inName) {}

        void run() override {

            while (!mTasks.empty() && mTasks.front().isReady()) {
                mTasks.front().run();
            }

            // this assumes that the scheduler is scheduled according the same policy
            /*if (!mTasks.empty()) {
                this->setRank(mTasks.front().getRank());
            }
            else {
                this->setRank(std::numeric_limits<rank_t>::max());
            }*/

        }

        bool addTask(ITask<rank_t>& inTask) {

            if (!inTask.init()) {
                return false;
            }

            mTasks.push_front(inTask);

            inTask.updateRank();

            auto& front = mTasks.front();

            if (&front == &inTask) {
                this->setRank(front.getRank());
            }

            return true;
        }

        bool isReady() const override {
            return (!mTasks.empty() && mTasks.front().isReady());
        }

        bool empty() const {
            return mTasks.empty();
        }

    private:
        ulink::List<ITask<rank_t>> mTasks;
    };

}