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

namespace ucosm {

    /**
     * @brief Real-time timer interface for platform abstraction.
     *
     * Provides a unified interface for hardware timers or threads
     * used by the real-time scheduler.
     *
     * @tparam task_t Task type that must implement run() method
     */
    template<typename task_t>
    struct IRTTimer {

        virtual ~IRTTimer() = default;
        /**
         * @brief Start timer counting and enable task management.
         */
        virtual void start() = 0;

        /**
         * @brief Stop timer and disable task management.
         */
        virtual void stop() = 0;

        /**
         * @brief Check if timer is actively running.
         * @return true if timer is counting
         */
        virtual bool isRunning() const = 0;

        /**
         * @brief Set timer period for task execution.
         * @param inDuration User-defined period unit.
         */
        virtual void setDuration(uint32_t inDuration) = 0;

        /**
         * @brief Temporarily disable timer.
         * Timer continues but won't execute tasks.
         */
        virtual void disable() = 0;

        /**
         * @brief Re-enable timer.
         */
        virtual void enable() = 0;

        /**
         * @brief Check if timer is available for task assignment.
         * @return true if no task assigned
         */
        bool isFree() const;

        /**
         * @brief Assign a task to this timer.
         * @param inTask Task to execute periodically
         * @return true if successfully assigned
         */
        bool setTask(task_t& inTask);

        /**
         * @brief Remove assigned task and stop timer.
         */
        void removeTask();

        /**
         * @brief Execute assigned task.
         * Runs task or stops timer if no task assigned.
         */
        void run();

    private:
        task_t* mTask = nullptr;
    };

    template<typename task_t>
    bool IRTTimer<task_t>::isFree() const {
        return (mTask == nullptr);
    }

    template<typename task_t>
    bool IRTTimer<task_t>::setTask(task_t& inTask) {
        if (mTask) {
            return false;
        }
        mTask = &inTask;
        return true;
    }

    template<typename task_t>
    void IRTTimer<task_t>::removeTask() {
        if (this->isRunning()) {
            this->stop();
        }
        mTask = nullptr;
    }

    template<typename task_t>
    void IRTTimer<task_t>::run() {
        if (mTask) {
            mTask->run();
        }
        else {
            this->stop();
        }
    }

}
