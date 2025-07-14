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
     * @brief Real-time timer interface template.
     *
     * This class provides a hardware abstraction layer for real-time timers
     * that can execute tasks at precise intervals. The timer manages a single
     * task and provides interrupt-based execution.
     *
     * @tparam task_t The type of task this timer can execute (must have run() method)
     */
    template<typename task_t>
    struct IRTTimer {

        /**
         * @brief Virtual destructor for proper cleanup of derived classes.
         */
        virtual ~IRTTimer() = default;

        // Core timer control interface
        /**
         * @brief Start the timer.
         * Timer will begin counting and trigger interrupts.
         */
        virtual void start() = 0;

        /**
         * @brief Stop the timer.
         * Timer will stop counting and disable interrupts.
         */
        virtual void stop() = 0;

        /**
         * @brief Check if timer is currently running.
         * @return true if timer is active, false otherwise
         */
        virtual bool isRunning() const = 0;

        /**
         * @brief Set the timer duration/period.
         * @param inDuration Duration in implementation-defined units (typically milliseconds)
         */
        virtual void setDuration(uint32_t inDuration) = 0;

        /**
         * @brief Disable timer interrupts.
         * Timer continues counting but won't trigger task execution.
         */
        virtual void disableInterruption() = 0;

        /**
         * @brief Enable timer interrupts.
         * Timer will trigger task execution when counter expires.
         */
        virtual void enableInterruption() = 0;

        // Task management interface
        /**
         * @brief Check if timer is available for task assignment.
         * @return true if no task is currently assigned, false otherwise
         */
        bool isFree() const;

        /**
         * @brief Assign a task to this timer.
         * @param inTask Reference to the task to be executed
         * @return true if task was successfully assigned, false if timer is busy
         */
        bool setTask(task_t& inTask);

        /**
         * @brief Remove the currently assigned task.
         * Timer will stop if it's running and no task will be executed.
         */
        void removeTask();

        /**
         * @brief Process timer interrupt.
         * This method should be called from the timer ISR.
         * Executes the assigned task or stops the timer if no task is assigned.
         */
        void processIT();

    private:
        task_t* mTask = nullptr;  ///< Currently assigned task (thread-safe)
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
        if (this->mIsRunning()) {
            this->stop();
        }
        mTask = nullptr;
    }

    template<typename task_t>
    void IRTTimer<task_t>::processIT() {
        if (mTask) {
            mTask->run();
        }
        else {
            this->stop();
        }
    }

}
