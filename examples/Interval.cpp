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
            TaskHandle h;

            // create tasks

            // if createTask returns true, the task has been created with success
            if(createTask(&PeriodicProcess::fastProcess, &h)){
                // set execution period, will be executed every 5ms
                h->setPeriod(5);
            }

            createTask(&PeriodicProcess::slowProcess);
        }

        void fastProcess(TaskHandle inHandle)
        {
            // do stuff
            std::cout << "fast" << std::endl;
        }

        void slowProcess(TaskHandle inHandle)
        {
            // do stuff
            std::cout << "slow" << std::endl;

            static bool sC = false;
            sC = !sC;

            if(sC) {
                // set delay manually to 1000 ms
                inHandle->setDelay(1000); // will restart in 1 s
            }else{
                // set delay manually to 500 ms
                inHandle->setDelay(500); // will restart in 500 ms
            }
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
