/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2024 Thomas AUBERT                                                *
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
 *                                                                                 *
 * github : https://github.com/ThomasAUB/ucosm                                     *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include "ulink.hpp"
#include <string_view>

namespace ucosm {

    template<typename _rank_t>
    struct ITask : ulink::Node<ITask<_rank_t>> {

        using rank_t = _rank_t;

        virtual void run() = 0;

        virtual bool init() { return true; }

        virtual void deinit() {}

        virtual std::string_view name() const { return ""; }

        void updateRank();

        void setRank(rank_t inRank);

        auto getRank() const;

    private:

        rank_t mRank = rank_t();

    };

    template<typename _rank_t>
    void ITask<_rank_t>::setRank(rank_t inRank) {
        mRank = inRank;
        updateRank();
    }

    template<typename _rank_t>
    auto ITask<_rank_t>::getRank() const { return mRank; }

    template<typename _rank_t>
    void ITask<_rank_t>::updateRank() {

        if (!this->isLinked()) {
            return;
        }

        if (this->prev->prev && mRank < this->prev->mRank) {
            // move task to the left

            auto* prevTask = this->prev->prev;

            while (prevTask->prev && mRank < prevTask->mRank) {
                prevTask = prevTask->prev;
            }

            // insert after prevTask

            this->remove();

            this->prev = prevTask;
            this->next = prevTask->next;
            this->prev->next = this;
            this->next->prev = this;

        }
        else if (this->next->next && this->next->mRank < mRank) {
            // move task to the right

            auto* nextTask = this->next->next;

            while (nextTask->next && nextTask->mRank < mRank) {
                nextTask = nextTask->next;
            }

            // insert before nextTask

            this->remove();

            this->next = nextTask;
            this->prev = nextTask->prev;
            this->next->prev = this;
            this->prev->next = this;

        }
    }

}