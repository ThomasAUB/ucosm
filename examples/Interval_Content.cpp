#include <iostream>

#include "kernel.h"
#include "task-handler.h"
#include "modules/Interval_M.h"
#include "modules/Content_M.h"
#include "modules/ModuleHub_M.h"
#include "modules/void_M.h"



#include <string.h>




////////////// time base ///////////////
#include <ctime>

tick_t getTick(){
    return time(0)*1000;
}

tick_t (*SysKernelData::sGetTick)() = &getTick;

////////////////////////////////////////


struct task_content{
	task_content():mCounter(0){}
	uint8_t mCounter;
	char mText[10];
};

using MyContent_t = Content_M<task_content>;

// defines the type of task properties, i.e. delay handling and buffer
using Modules = ModuleHub_M< Interval_M, MyContent_t>; 


// PeriodicProcess is an example of class containing the tasks
// TaskHandler's arguments : 
//	  - PeriodicProcess : the container itself using CRTP technique.
//	  - task_trait_t : the type of task handled by PeriodicProcess.
//	  - 2 : the max number of simultaneous tasks. 

class PeriodicProcess : public TaskHandler< PeriodicProcess, Modules, 2 >
{
	public:

		PeriodicProcess()
		{

			TaskHandle fastHandle;

			// create tasks
			if(createTask(&PeriodicProcess::fastProcess, &fastHandle)){
				strcpy(fastHandle->getModule<MyContent_t>()->getContent().mText, "fast");
			}

			if(createTask(&PeriodicProcess::slowProcess, &slowHandle)){
				strcpy(slowHandle->getModule<MyContent_t>()->getContent().mText, "slow");
			}
			
		}

		void fastProcess(TaskHandle inHandle)
		{
			// do stuff
			std::cout << inHandle->getModule<MyContent_t>()->getContent().mText << std::endl;
			
			// if the task slowProcess is not alive anymore : delete fastProcess
			if(!slowHandle()){
				std::cout << "delete fastProcess" << std::endl;
				deleteTask(inHandle);
			}
		}

		void slowProcess(TaskHandle inHandle)
		{
			// do stuff
			std::cout << inHandle->getModule<MyContent_t>()->getContent().mText << std::endl;
			inHandle->setDelay(1000); // will restart in 1 s

			// if the task has been executed 5 times : delete slowProcess
			if(tinHandle->getModule<MyContent_t>()->getContent().mCounter++ >= 5)
			{
				std::cout << "delete slowProcess" << std::endl;
				deleteTask(inHandle);
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
