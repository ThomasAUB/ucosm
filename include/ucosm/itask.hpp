#pragma once

#include "ulink.hpp"
#include <string_view>

namespace ucosm {

    struct ITask : ulink::Node<ITask> {

        using rank_t = int;

        ITask(std::string_view inName = "") :
            mName(inName) {}

        virtual bool init() { return true; }

        virtual void deinit() {}

        virtual void run() = 0;

        virtual rank_t getRank() const = 0;

        std::string_view name() const { return mName; }

    protected:
        rank_t mRank = 0;
        void setRank(rank_t inRank) {
            mRank = inRank;
            updateRank();
        }

        void updateRank() {

            const rank_t r = getRank();

            if (this->prev->prev && r < this->prev->getRank()) {
                // move task to the left
                this->remove();

                auto* prevTask = this->prev;

                while (prevTask->prev && r < prevTask->prev->getRank()) {
                    prevTask = prevTask->prev;
                }

                // insert before prevTask
                next = prevTask;
                prev = prevTask->prev;
                next->prev = this;

                if (prev) {
                    prev->next = this;
                }

            }
            else if (this->next->next && this->next->getRank() < r) {
                // move task to the right
                this->remove();

                auto* nextTask = this->next;

                while (nextTask->next && nextTask->next->getRank() < r) {
                    nextTask = nextTask->next;
                }

                // insert after nextTask
                prev = nextTask;
                next = nextTask->next;
                prev->next = this;

                if (next) {
                    next->prev = this;
                }
            }
        }

    private:

        std::string_view mName;

    };

}