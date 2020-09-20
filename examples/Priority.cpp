#include <iostream>

#include "kernel.h"
#include "task-handler.h"
#include "modules/Priority_M.h"
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
//	  - Priority_M : the type of task handled by PeriodicProcess.
//	  - 2 : the max number of simultaneous tasks. 

class PeriodicProcess : public TaskHandler< PeriodicProcess, Priority_M, 2 >
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

		void highPriorityProcess()
		{
			// do stuff
			std::cout << "high prio" << std::endl;
		}

		void lowPriorityProcess()
		{
			// do stuff
			std::cout << "low prio" << std::endl;
		}
	
};












// instantiation of the master scheduler
//  Kernel's argument :
//	  - void_M : defines the handler's properties, i.e. no properties 
//		(the handler traits are the same as the task traits).
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
