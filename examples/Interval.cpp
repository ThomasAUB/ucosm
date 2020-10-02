#include <iostream>

#include "kernel.h"
#include "task-handler.h"
#include "modules/Interval_M.h"
#include "modules/void_M.h"



////////////// time base ///////////////
#include <ctime>

tick_t getTick(){
    return time(0)*1000;
}

tick_t (*SysKernelData::sGetTick)() = &getTick;

////////////////////////////////////////





// PeriodicProcess is an example of class containing the tasks
// TaskHandler's arguments : 
//	  - PeriodicProcess : the container itself using CRTP technique.
//	  - Module : the type of task handled by PeriodicProcess.
//	  - 2 : the max number of simultaneous tasks. 

class PeriodicProcess : public TaskHandler< PeriodicProcess, Interval_M, 2 >
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
//	  - void_M : defines the handler's properties, i.e. no properties
//	  - 1 : the max number of simultaneous handlers.
Kernel<void_M, 1> kernel;


PeriodicProcess periodicProcess;

int main()
{

	// adding periodicProcess to the master scheduler
	kernel.addHandler(&periodicProcess);
		
	while(1)
	{
		kernel.schedule();
	}
	
	return 0;
}