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

#include "itask.hpp"
#include <stdint.h>

namespace ucosm {

    using tick_t = uint32_t;

    /**
     * @brief Periodic task.
     */
    struct IPeriodicTask : ITask<tick_t> {

        using get_tick_t = tick_t(*)();

        IPeriodicTask() :
            mPeriod(0) {}

        IPeriodicTask(tick_t inPeriod) :
            mPeriod(inPeriod) {}

        /**
         * @brief Delay task.
         *
         * @param inDelay Delay value.
         */
        void setDelay(tick_t inDelay) { this->setRank(sGetTick() + inDelay); }

        /**
         * @brief Set the task period.
         *
         * @param inPeriod Period value.
         */
        void setPeriod(tick_t inPeriod) { mPeriod = inPeriod; }

        /**
         * @brief Get the task period.
         *
         * @return tick_t Period  value.
         */
        tick_t getPeriod() const { return mPeriod; }

        /**
         * @brief Set the tick function.
         *
         * @param inGetTick Get tick function pointer.
         */
        static void setTickFunction(get_tick_t inGetTick) { sGetTick = inGetTick; }

        /**
         * @brief Get the tick value.
         *
         * @return tick_t Tick value.
         */
        static tick_t getTick() { return sGetTick(); }

    protected:

        tick_t mPeriod;

    private:

        virtual void periodicRun() = 0;

        void run() override {
            periodicRun();
            this->setRank(this->getRank() + mPeriod);
        }

        inline static get_tick_t sGetTick = +[] { return tick_t(); };

    };

}