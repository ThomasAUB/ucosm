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

#include "iperiodic_task.hpp"

namespace ucosm {

    /**
     * @brief Completely fair scheduler.
     */
    struct ICFSTask : IPeriodicTask {

        using priority_t = uint8_t;

        /**
         * @brief Set the task priority.
         *
         * @param inPriority Priority value between 0 (highest) and 16 (lowest)
         */
        void setPriority(priority_t inPriority) {
            if (inPriority > 16) {
                inPriority = 16;
            }
            mPriority = inPriority;
        }

        /**
         * @brief Get the task priority.
         *
         * @return priority_t Priority value.
         */
        priority_t getPriority() const { return mPriority; }

    private:

        void onPeriodElapsed() override final {}

        virtual void execute() = 0;

        void run() override {

            auto timeStamp = getTick();

            execute();

            tick_t duration = 1 + getTick() - timeStamp;
            duration <<= mPriority;

            timeStamp += (duration > this->mPeriod) ? duration : this->mPeriod;
            this->setRank(timeStamp);
        }

        priority_t mPriority = 2;

    };

}