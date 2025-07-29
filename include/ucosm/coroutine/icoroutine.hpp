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

#include "ucosm/periodic/iperiodic_task.hpp"
#include "context.hpp"
#include <array>
#include <string_view>

namespace ucosm {

    // Forward declaration
    class CoroutineScheduler;

    /**
     * @brief Coroutine state enumeration.
     */
    enum class CoroutineState : uint8_t {
        Created,    // Coroutine created but not started
        Running,    // Coroutine is currently executing
        Suspended,  // Coroutine yielded and waiting to resume
        Waiting,    // Coroutine is waiting for a condition or timeout
        Finished    // Coroutine completed execution
    };

    /**
     * @brief Base class for coroutines with automatic context switching.
     * 
     * This class provides a coroutine implementation that integrates with the
     * µCosm scheduler framework. Coroutines can yield execution and resume
     * at the same point, enabling cooperative multitasking with explicit
     * control flow.
     * 
     * @tparam StackSize Size of the coroutine stack in bytes
     */
    template<std::size_t StackSize = 8192>
    class ICoroutine : public IPeriodicTask {
    public:
        static_assert(StackSize >= getMinStackSize(), 
                     "Stack size must be at least minimum required size");
        static_assert((StackSize & (getStackAlignment() - 1)) == 0,
                     "Stack size must be aligned to platform requirements");

        /**
         * @brief Construct a new coroutine.
         * @param period Initial period for the periodic task (0 = run as fast as possible)
         */
        explicit ICoroutine(tick_t period = 0) 
            : IPeriodicTask(period)
            , mState(CoroutineState::Created)
            , mWaitUntil(0)
            , mCurrentScheduler(nullptr) {
            
            // Initialize stack with pattern for debugging
            mStack.fill(0xCC);
            
            // Initialize the coroutine context
            initializeContext(&mCoroutineContext, mStack.data(), StackSize,
                            &ICoroutine::coroutineEntryPoint, this);
        }

        /**
         * @brief Virtual destructor.
         */
        virtual ~ICoroutine() = default;

        /**
         * @brief Get the current state of the coroutine.
         * @return Current coroutine state
         */
        CoroutineState getState() const { return mState; }

        /**
         * @brief Check if the coroutine has finished execution.
         * @return true if finished, false otherwise
         */
        bool isFinished() const { return mState == CoroutineState::Finished; }

        /**
         * @brief Get the name of this coroutine for debugging.
         * @return String view with the coroutine name
         */
        std::string_view name() override { return "Coroutine"; }

    protected:
        /**
         * @brief Pure virtual function that contains the coroutine logic.
         * 
         * This function should contain the main logic of the coroutine.
         * It can call yield() and wait() functions to suspend execution.
         */
        virtual void execute() = 0;

        /**
         * @brief Yield execution to other tasks.
         * 
         * The coroutine will be rescheduled according to its period setting.
         * Control returns to the scheduler and the coroutine will resume
         * at this point when next scheduled.
         */
        void yield() {
            if (mCurrentScheduler) {
                contextSwitch(&mCoroutineContext, &mSchedulerContext);
            }
        }

        /**
         * @brief Wait for a specified number of ticks.
         * @param ticks Number of scheduler ticks to wait
         */
        void waitFor(tick_t ticks);

        /**
         * @brief Wait until a specific tick count is reached.
         * @param target_tick Target tick count to wait for
         */
        void waitUntil(tick_t target_tick);

        /**
         * @brief Wait until a condition becomes true, checking every period.
         * @param condition Function or lambda that returns true when waiting should stop
         * @param check_period How often to check the condition (in ticks)
         */
        template<typename Predicate>
        void waitUntil(Predicate&& condition, tick_t check_period = 1) {
            while (!condition()) {
                waitFor(check_period);
            }
        }

        /**
         * @brief Get access to the current scheduler (if available).
         * @return Pointer to scheduler or nullptr if not running in scheduler
         */
        CoroutineScheduler* getCurrentScheduler() const { 
            return mCurrentScheduler; 
        }

    private:
        // Allow CoroutineScheduler to access private methods
        friend class CoroutineScheduler;

        /**
         * @brief Entry point for the coroutine (called by context switching).
         * @param self Pointer to the coroutine instance
         */
        static void coroutineEntryPoint(void* self) {
            auto* coroutine = static_cast<ICoroutine*>(self);
            coroutine->mState = CoroutineState::Running;
            
            try {
                coroutine->execute();
            } catch (...) {
                // Handle exceptions gracefully
            }
            
            coroutine->mState = CoroutineState::Finished;
            
            // Return control to scheduler
            if (coroutine->mCurrentScheduler) {
                contextSwitch(&coroutine->mCoroutineContext, 
                            &coroutine->mSchedulerContext);
            }
        }

        /**
         * @brief Implementation of IPeriodicTask::run() - called by scheduler.
         */
        void run() override;

        /**
         * @brief Set the scheduler reference (used internally).
         * @param scheduler Pointer to the coroutine scheduler
         */
        void setScheduler(CoroutineScheduler* scheduler) {
            mCurrentScheduler = scheduler;
        }

        // Coroutine state
        CoroutineState mState;
        tick_t mWaitUntil;
        CoroutineScheduler* mCurrentScheduler;

        // Context switching
        Context mCoroutineContext;
        Context mSchedulerContext;

        // Stack for the coroutine
        alignas(getStackAlignment()) std::array<uint8_t, StackSize> mStack;
    };

} // namespace ucosm