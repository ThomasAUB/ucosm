#include <iostream>

#include "/uCoSM/kernel.h"

#include "/uCoSM/modules.h"









////////////// time base ///////////////
#include <ctime>

tick_t getTick(){
    return time(0)*1000;
}

tick_t (*SysKernelData::sGetTick)() = &getTick;

////////////////////////////////////////







using namespace ucosm_modules;









// defines the type of task properties, i.e. delay handling
using task_module_t = Modules< Delay >; 


// PeriodicProcess is an example of class containing the tasks
// TaskHandler's arguments : 
//	  - PeriodicProcess : the container itself using CRTP technique.
//	  - task_trait_t : the type of task handled by PeriodicProcess.
//	  - 2 : the max number of simultaneous tasks. 

class PeriodicProcess : public TaskHandler< PeriodicProcess, task_module_t, 2 >
{
	public:

		PeriodicProcess()
		{

			// create tasks
			createTask(&PeriodicProcess::fastProcess);

			createTask(&PeriodicProcess::slowProcess);
		
		}

		void fastProcess()
		{
			// do stuff
			std::cout << "fast" << std::endl;
			thisTaskHandle()->setDelay(5); // will restart in 5 ms
		}

		void slowProcess()
		{
			// do stuff
			std::cout << "slow" << std::endl;
			thisTaskHandle()->setDelay(1000); // will restart in 1 s
		}
	
};











// instantiation of the master scheduler
//  Kernel's argument :
//	  - Traits<> : defines the handler's properties, i.e. no properties
//	  - 1 : the max number of simultaneous handlers.
Kernel<Modules<>, 1> kernel;


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

