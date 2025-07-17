#pragma once

#include "iperiodic_task.hpp"
#include <string>


// Internal state enum for better type safety
#define UCOSM_STATE_ENTRY (-1)
#define UCOSM_STATE_END   (-2)


// Protected macro definitions with validation
#define UCOSM_START                                         \
    do {                                                    \
        switch(this->mLine) {                               \
        case UCOSM_STATE_ENTRY: {

#define UCOSM_WAIT(tick)                                    \
            this->mPeriodBackup = this->getPeriod();        \
            this->setPeriod(static_cast<tick_t>(tick));     \
            this->mLine = __LINE__;                         \
            return;                                         \
        }                                                   \
        case __LINE__: {                                    \
            this->setPeriod(this->mPeriodBackup);

#define UCOSM_YIELD                                         \
            this->mPeriodBackup = this->getPeriod();        \
            this->setPeriod(0);                             \
            this->mLine = __LINE__;                         \
            return;                                         \
        }                                                   \
        case __LINE__: {                                    \
            this->setPeriod(this->mPeriodBackup);

#define UCOSM_LOOP                                          \
            this->mLine = UCOSM_STATE_ENTRY;                \
            return;                                         \
        }                                                   \
        }                                                   \
    } while(0);                                             \
    return;

#define UCOSM_END                                           \
            this->mLine = UCOSM_STATE_END;                  \
            this->removeTask();                             \
            return;                                         \
        }                                                   \
        default:                                            \
            /* Invalid state - task corrupted */            \
            this->removeTask();                             \
            return;                                         \
        }                                                   \
    } while(0);


// Additional utility macros  
#define UCOSM_DELAY_MS(ms) UCOSM_WAIT(ms)

#define UCOSM_WAIT_UNTIL(condition, check_period)           \
        if(!(condition)) {                                  \
            UCOSM_WAIT(check_period);                       \
            if (!(condition)) {                             \
                return;                                     \
            }                                               \
        }


// Note: UCOSM_WAIT_UNTIL is complex to implement with the current macro system.
// For condition-based waiting, use a loop with UCOSM_YIELD:
//   if (!condition) { UCOSM_YIELD }
// Or implement timeout checking manually in your task.

namespace ucosm {

    /**
     * @brief Resumable task with coroutine-like behavior.
     *
     * Allows writing tasks that can yield execution and resume later,
     * similar to cooperative multitasking or coroutines.
     *
     * Features:
     * - Zero heap allocation
     * - Minimal memory overhead (8 bytes)
     * - Cooperative multitasking
     * - Time-based delays
     * - Loop constructs
     * - Safe macro system
     *
     * Usage:
     * @code
     * struct MyTask : IResumableTask {
     *     void run() override {
     *         UCOSM_START
     *             // Initialization code
     *             UCOSM_WAIT(1000)  // Wait 1 second
     *             // More code
     *             UCOSM_YIELD       // Yield to other tasks
     *             // Final code
     *         UCOSM_END
     *     }
     * };
     * @endcode
     */
    struct ICoroutine : IPeriodicTask {
        /// Default constructor
        ICoroutine() = default;

        /// Constructor with initial period
        explicit ICoroutine(tick_t period) : IPeriodicTask(period) {}

    protected:

        /**
         * @brief Get current time in milliseconds.
         * Override this method to provide time source for UCOSM_WAIT_UNTIL
         * @return Current time in milliseconds
         */
        virtual uint32_t getCurrentTime() const {
            // Simple fallback: use a static counter
            // In real applications, override this with actual time source
            static uint32_t counter = 0;
            return ++counter * 10; // Simulate 10ms increments
        }

    public:
        /**
         * @brief Check if task state is valid.
         * @return true if task is in valid state
         */
        bool isStateValid() const {
            return mLine >= UCOSM_STATE_ENTRY; // -1 (entry) and above are valid
        }

        /**
         * @brief Reset task to initial state.
         * Useful for restarting completed tasks.
         */
        void resetState() {
            mLine = UCOSM_STATE_ENTRY;
            mPeriodBackup = 0;
        }

        /**
         * @brief Get current execution state for debugging.
         * @return Current line number or state constant
         */
        int getCurrentState() const {
            return mLine;
        }

        /**
         * @brief Get a string description of the current state
         * @return String representation of current state
         */
        std::string getStateDescription() const {
            if (mLine == UCOSM_STATE_ENTRY) return "STATE_ENTRY";
            if (mLine == UCOSM_STATE_END) return "STATE_END";
            return "LINE_" + std::to_string(mLine);
        }

        /**
         * @brief Helper for implementing timeout logic
         * @param start_time Reference to store start time (use static variable)
         * @param timeout_ms Timeout in milliseconds
         * @return true if timeout has occurred
         */
        bool hasTimedOut(uint32_t& start_time, uint32_t timeout_ms) {
            if (start_time == 0) {
                start_time = getCurrentTime();
                return false;
            }
            uint32_t elapsed = getCurrentTime() - start_time;
            if (elapsed >= timeout_ms) {
                start_time = 0; // Reset for next use
                return true;
            }
            return false;
        }

    protected:
        uint32_t mPeriodBackup = 0;  ///< Backup of original period before wait
        int mLine = UCOSM_STATE_ENTRY;  ///< Current execution line/state

        friend struct ResumableTaskDebugger; // For debugging support
    };

    /**
     * @brief Debug helper for resumable tasks.
     * Provides introspection capabilities for development and testing.
     */
    struct ResumableTaskDebugger {
        static const char* getStateName(const ICoroutine& task) {
            switch (task.mLine) {
                case UCOSM_STATE_ENTRY: return "ENTRY";
                case UCOSM_STATE_END: return "ENDED";
                default: return task.mLine > 0 ? "WAITING" : "INVALID";
            }
        }

        static bool isWaiting(const ICoroutine& task) {
            return task.mLine > 0;
        }

        static bool isEnded(const ICoroutine& task) {
            return task.mLine == UCOSM_STATE_END;
        }
    };

}