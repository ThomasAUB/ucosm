#pragma once

#include <stdint.h>
#include "ucosm/core/ischeduler.hpp"
#include "ucosm/periodic/iperiodic_task.hpp"

namespace ucosm {

    /**
     * @brief Real-time scheduler.
     */
    struct RTScheduler : IScheduler<IPeriodicTask, ITask<uint8_t>> {

        struct ITimer {

            virtual void start() = 0;
            virtual void stop() = 0;
            virtual bool isRunning() const = 0;
            virtual void setDuration(uint32_t inDuration) = 0;

            void processIT() {
                if (mScheduler) {
                    mScheduler->run();
                }
            }

        private:
            friend struct RTScheduler;
            RTScheduler* mScheduler = nullptr;
        };

        RTScheduler(ITimer& inTimer);

        /**
         *@brief Add a task to the scheduler. Must be called from mainloop.
         *
         * @param inTask
         * @return true
         * @return false
         */
        bool addTask(IPeriodicTask& inTask) override;

    protected:
        void run() override;
        using base_t = IScheduler<IPeriodicTask, ITask<uint8_t>>;
        ITimer& mTimer;
        volatile bool mISCriticalSection = false;
    };

}