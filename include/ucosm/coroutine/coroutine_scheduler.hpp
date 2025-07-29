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

#include "ucosm/periodic/periodic_scheduler.hpp"

namespace ucosm {

    // Forward declaration
    template<std::size_t StackSize>
    class ICoroutine;

    /**
     * @brief Specialized scheduler for coroutines with tick tracking.
     * 
     * This scheduler extends the periodic scheduler to provide additional
     * functionality needed by coroutines, including tick counting for
     * time-based waiting operations.
     */
    class CoroutineScheduler : public PeriodicScheduler<> {
    public:
        using tick_func_t = IPeriodicTask::tick_t(*)();
        using tick_t = IPeriodicTask::tick_t;
        
        /**
         * @brief Construct a new coroutine scheduler.
         * @param tick_func Function that returns current system tick
         * @param idle_task Optional idle task to run when no coroutines are ready
         */
        explicit CoroutineScheduler(tick_func_t tick_func, idle_task_t idle_task = nullptr)
            : PeriodicScheduler<>(tick_func, idle_task)
            , mCurrentTick(0) {}

        /**
         * @brief Add a coroutine to the scheduler.
         * @param coroutine Reference to the coroutine to add
         * @return true if successfully added, false otherwise
         */
        template<std::size_t StackSize>
        bool addCoroutine(ICoroutine<StackSize>& coroutine) {
            // Set the scheduler reference in the coroutine
            coroutine.setScheduler(this);
            return this->addTask(coroutine);
        }

        /**
         * @brief Get the current tick count.
         * @return Current tick value
         */
        tick_t getCurrentTick() const { return mCurrentTick; }

        /**
         * @brief Run the scheduler for one iteration.
         * 
         * This overrides the base scheduler to update the current tick
         * and handle coroutine-specific logic.
         */
        void run() override {
            // Update current tick
            mCurrentTick = this->mGetTick();
            
            // Run the base scheduler logic
            PeriodicScheduler<>::run();
        }

        /**
         * @brief Get the number of active coroutines.
         * @return Number of coroutines in the scheduler
         */
        std::size_t getCoroutineCount() const {
            return this->size();
        }

        /**
         * @brief Check if any coroutines are still running or waiting.
         * @return true if there are active coroutines, false otherwise
         */
        bool hasActiveCoroutines() const {
            return !this->empty();
        }

        /**
         * @brief Remove all finished coroutines from the scheduler.
         * @return Number of coroutines removed
         */
        std::size_t cleanupFinished() {
            std::size_t removed = 0;
            // Note: Finished coroutines remove themselves automatically
            // This method is provided for consistency but may not be needed
            return removed;
        }

    private:
        tick_t mCurrentTick;
    };


} // namespace ucosm