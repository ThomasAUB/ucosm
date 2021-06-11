#include <iostream>

#include "task_object.h"
#include "task_function.h"
#include "modules/interval_m.h"



////////////// time base ///////////////
#include "../../examples/time_base.h"
tick_t (*UcosmSysData::sGetTick)() = &getTick;
////////////////////////////////////////






// PeriodicProcess is an example of class containing the tasks
// TaskHandler's arguments :
//    - PeriodicProcess : the container itself using CRTP technique.
//    - 2 : the max number of simultaneous tasks.
//    - Interval_M : the type of task handled by PeriodicProcess.

class PeriodicProcess : public TaskFunction< PeriodicProcess, 2, Interval_M >
{
    public:

        PeriodicProcess()
        {

            // create tasks
            createTask(&PeriodicProcess::fastProcess);

            createTask(&PeriodicProcess::slowProcess);

        }

        void fastProcess(TaskHandle inHandle)
        {
            // do stuff
            std::cout << "fast" << std::endl;
            inHandle->setDelay(5); // will restart in 5 ms
        }

        void slowProcess(TaskHandle inHandle)
        {
            // do stuff
            std::cout << "slow" << std::endl;
            inHandle->setDelay(1000); // will restart in 1 s
        }

};







// instantiation of the master scheduler
//  Kernel's argument :
//    - 1 : the max number of simultaneous handlers.
TaskObject<1> kernel;


PeriodicProcess periodicProcess;

int main()
{

    // adding periodicProcess to the master scheduler
    kernel.addTask(&periodicProcess);

    while(1)
    {
        kernel.schedule();
    }

    return 0;
}
