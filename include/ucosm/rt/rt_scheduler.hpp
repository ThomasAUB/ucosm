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

#include <stdint.h>
#include "irt_timer.hpp"
#include "ucosm/core/ischeduler.hpp"
#include "ucosm/periodic/iperiodic_task.hpp"

namespace ucosm {

    /**
     * @brief Real-time scheduler.
     */
    struct RTScheduler : IScheduler<IPeriodicTask, ITask<uint8_t>> {

        using ITimer = IRTTimer<ITask<uint8_t>>;

        bool setTimer(ITimer& inTimer);

        bool addTask(IPeriodicTask& inTask) override;

        bool addTask(IPeriodicTask& inTask, IPeriodicTask::tick_t inDelay);

        ~RTScheduler();

    protected:

        void delay(uint32_t inDelay);

        uint32_t mCounter = 0;
        void run() override;
        using base_t = IScheduler<IPeriodicTask, ITask<uint8_t>>;
        ITimer* mTimer = nullptr;
    };

}