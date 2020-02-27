#include <iostream>

#include "/uCoSM/kernel.h"

#include "/uCoSM/traits.h"









////////////// time base ///////////////
#include <ctime>

tick_t getTick(){
    return time(0)*1000;
}

tick_t (*SysKernelData::sGetTick)() = &getTick;

////////////////////////////////////////







using namespace ucosm_traits;









// defines the type of task properties, i.e. delay handling and buffer
using task_trait_t = Traits< Delay, Buffer<uint8_t, 1> >; 


// PeriodicProcess is an example of class containing the tasks
// TaskHandler's arguments : 
//	  - PeriodicProcess : the container itself using CRTP technique.
//	  - task_trait_t : the type of task handled by PeriodicProcess.
//	  - 2 : the max number of simultaneous tasks. 

class PeriodicProcess : public TaskHandler< PeriodicProcess, task_trait_t, 2 >
{
	public:

		PeriodicProcess()
		{

			// create tasks
			createTask(&PeriodicProcess::fastProcess);

			createTask(&PeriodicProcess::slowProcess, &slowHandle);

			slowExeCount = &thisTaskHandle()->getBuffer()[0];
			*slowExeCount = 0; // initialize buffer value
			
		}

		void fastProcess()
		{
			// do stuff
			std::cout << "fast" << std::endl;

			// if the task slowProcess is not alive anymore : delete fastProcess
			if(!slowHandle){
				std::cout << "delete fastProcess" << std::endl;
				deleteTask(thisTaskHandle());
			}
		}

		void slowProcess()
		{
			// do stuff
			std::cout << "slow" << std::endl;
			thisTaskHandle()->setDelay(1000); // will restart in 1 s

			// if the task has been executed 5 times : delete slowProcess
			if(*slowExeCount++ >= 5)
			{
				std::cout << "delete slowProcess" << std::endl;
				deleteTask(thisTaskHandle());
			}
		}

	private:

		TaskHandle slowHandle;
		uint8_t *slowExeCount;
};













// instantiation of the master scheduler
//  Kernel's argument :
//	  - Traits<> : defines the handler's properties, i.e. no properties
//	  - 1 : the max number of simultaneous handlers.
Kernel<Traits<>, 1> kernel;


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

