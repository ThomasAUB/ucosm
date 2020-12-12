#include <iostream>

#include "kernel.h"
#include "task-handler.h"
#include "modules/Priority_M.h"



////////////// time base ///////////////
#include "time_base.h"
tick_t (*SysKernelData::sGetTick)() = &getTick;
////////////////////////////////////////







// PeriodicProcess is an example of class containing the tasks
// TaskHandler's arguments : 
//	  - PeriodicProcess : the container itself using CRTP technique.
//	  - 2 : the max number of simultaneous tasks. 
//	  - Priority_M : the type of task handled by PeriodicProcess.

class PeriodicProcess : public TaskHandler< PeriodicProcess, 2, Priority_M >
{
	public:

		PeriodicProcess()
		{
			// create handles
			TaskHandle highPrio, lowPrio;

			// create tasks
			createTask(&PeriodicProcess::highPriorityProcess, &highPrio);

			createTask(&PeriodicProcess::lowPriorityProcess, &lowPrio);

			// set the corresponding priorities
			highPrio->setPriority(1); // HighPriorityProcess will be executed every mainloop cycles
			lowPrio->setPriority(255); // LowPriorityProcess will be executed every 255 mainloop cycles
		}

		void highPriorityProcess(TaskHandle h)
		{
			// do stuff
			std::cout << "high prio" << std::endl;
		}

		void lowPriorityProcess(TaskHandle h)
		{
			// do stuff
			std::cout << "low prio" << std::endl;
		}
	
};












// instantiation of the master scheduler
//  Kernel's argument :
//	  - 1 : the max number of simultaneous handlers.
Kernel<1> kernel;


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
