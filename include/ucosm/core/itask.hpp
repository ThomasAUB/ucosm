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

    /**
     * @brief Task interface.
     *
     * @tparam rank_t Type used to sort tasks execution.
     */
    template<typename _rank_t>
    struct ITask : ulink::Node<ITask<_rank_t>> {

        using rank_t = _rank_t;

        virtual ~ITask() = default;

        /**
         * @brief Runs the task.
         * Typically called by the scheduler when the task is ready.
         */
        virtual void run() = 0;

        /**
         * @brief Initializes the task.
         * Typically called by the scheduler when the task is added.
         *
         * @return true if the task was successfully initialized.
         * @return false otherwise.
         */
        virtual bool init() { return true; }

        /**
         * @brief Deinitializes the task.
         * Called when the task is removed.
         */
        virtual void deinit() {}

        /**
         * @brief Removes the task from the scheduler.
         */
        void removeTask();

        /**
         * @brief Returns the name of the task.
         *
         * @return std::string_view Task name.
         */
        virtual std::string_view name() { return ""; }

        /**
         * @brief Updates the task position in the list according to its rank value.
         *
         * The list must be the one the task is currently linked to. The task is
         * moved with the list iterators so that the start and end sentinels are
         * never accessed as tasks.
         *
         * @param ioList List the task belongs to.
         * @return true if the task was moved in the list
         * @return false otherwise.
         */
        bool updateRank(ulink::List<ITask>& ioList);

        /**
         * @brief Sets the task rank value.
         *
         * @param inRank inRank New rank value.
         */
        void setRank(rank_t inRank);

        /**
         * @brief Gets the task rank value.
         *
         * @return rank_t Rank value.
         */
        rank_t getRank() const;

    private:

        template<typename T>
        friend class ulink::List;

        using ulink::Node<ITask<rank_t>>::remove;

        rank_t mRank = rank_t();
    };

    template<typename rank_t>
    void ITask<rank_t>::removeTask() {
        if (this->isLinked()) {
            deinit();
        }
        ulink::Node<ITask<rank_t>>::remove();
    }

    template<typename rank_t>
    void ITask<rank_t>::setRank(rank_t inRank) {
        mRank = inRank;
    }

    template<typename rank_t>
    rank_t ITask<rank_t>::getRank() const {
        return mRank;
    }

    template<typename rank_t>
    bool ITask<rank_t>::updateRank(ulink::List<ITask<rank_t>>& ioList) {

        if (!this->isLinked()) {
            return false;
        }

        using iterator_t = typename ulink::List<ITask<rank_t>>::iterator;

        const iterator_t begin = ioList.begin();
        const iterator_t end = ioList.end();

        const iterator_t self(this);

        // The list boundaries are detected by comparing against begin() and
        // end() rather than by inspecting the neighbor links : the sentinel
        // nodes are plain ulink::Node and must never be read as tasks.

        const bool isFirst = (self == begin);

        iterator_t left = self;
        if (!isFirst) {
            --left;
        }

        iterator_t right = self;
        ++right;

        const bool isLast = (right == end);

        // Already in the correct slot: no work.
        const bool leftSorted = (isFirst || left->getRank() <= mRank);
        const bool rightSorted = (isLast || mRank <= right->getRank());

        if (leftSorted && rightSorted) {
            return false;
        }

        iterator_t pos = self;

        if (!leftSorted) {
            // move task to the left, stop on the first task ranked before us
            pos = left;
            while (pos != begin) {
                iterator_t candidate = pos;
                --candidate;
                if (candidate->getRank() <= mRank) {
                    break;
                }
                pos = candidate;
            }
        }
        else {
            // move task to the right, stop on the first task ranked after us
            pos = right;
            ++pos;
            while (pos != end && pos->getRank() < mRank) {
                ++pos;
            }
        }

        // relinks the task before pos, unlinking it from its current spot
        ioList.insert_before(pos, *this);

        return true;
    }

}