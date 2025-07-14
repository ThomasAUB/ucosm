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
         * @return true if the task was moved in the list
         * @return false otherwise.
         */
        bool updateRank();

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
    bool ITask<rank_t>::updateRank() {

        if (!this->isLinked()) {
            return false;
        }

        auto* prevTask = this->prev->prev;
        auto* nextTask = this->next->next;

        if (prevTask && mRank < this->prev->mRank) {
            // move task to the left

            while (prevTask->prev && mRank < prevTask->mRank) {
                prevTask = prevTask->prev;
            }

            // insert after prevTask

            this->ulink::Node<ITask<rank_t>>::remove();

            this->prev = prevTask;
            this->next = prevTask->next;
            this->prev->next = this;
            this->next->prev = this;

        }
        else if (nextTask && this->next->mRank < mRank) {
            // move task to the right

            while (nextTask->next && nextTask->mRank < mRank) {
                nextTask = nextTask->next;
            }

            // insert before nextTask

            this->ulink::Node<ITask<rank_t>>::remove();

            this->next = nextTask;
            this->prev = nextTask->prev;
            this->next->prev = this;
            this->prev->next = this;

        }
        else {
            return false;
        }

        return true;
    }

}