#pragma once

#include "ulink.hpp"
#include <string_view>

namespace ucosm {

    template<typename rank_t>
    struct ITask : ulink::Node<ITask<rank_t>> {

        ITask(std::string_view inName = "") :
            mName(inName) {}

        virtual bool init() { return true; }

        virtual void deinit() {}

        virtual void run() = 0;

        std::string_view name() const { return mName; }

        virtual bool isReady() const = 0;

        void updateRank() {

            if (this->prev && this->prev->prev && mRank < this->prev->mRank) {
                // move task to the left

                auto* prevTask = this->prev;

                while (prevTask->prev && mRank < prevTask->prev->mRank) {
                    prevTask = prevTask->prev;
                }

                this->remove();

                // insert before prevTask
                this->next = prevTask;
                this->prev = prevTask->prev;
                this->next->prev = this;

                if (this->prev) {
                    this->prev->next = this;
                }

            }
            else if (this->next && this->next->next && this->next->mRank < mRank) {
                // move task to the right

                auto* nextTask = this->next;

                while (nextTask->next && nextTask->next->mRank < mRank) {
                    nextTask = nextTask->next;
                }

                this->remove();

                // insert after nextTask
                this->prev = nextTask;
                this->next = nextTask->next;
                this->prev->next = this;

                if (this->next) {
                    this->next->prev = this;
                }
            }
        }

        rank_t getRank() const { return mRank; }

    protected:

        rank_t mRank = rank_t();

        void setRank(rank_t inRank) {
            mRank = inRank;
            updateRank();
        }



    private:

        std::string_view mName;

    };

}