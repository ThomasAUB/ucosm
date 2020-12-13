[![Build Status](https://travis-ci.com/ThomasAUB/uCoSM.svg?branch=master)](https://travis-ci.com/ThomasAUB/uCoSM)

# uCoSM
Module based cooperative scheduler for microcontroler (Beta)

  uCoSM is a lightweight and modular embedded scheduler designed in C++ (c++14 or above). 
  
  Its features are :
  
    - Dynamically add and remove schedulable items such as function and class.
    - No use of heap memory.
    - Creation of custom task and handler properties called "modules".
    - TaskHandle management.
    - Minimal use of virtual functions.
  
  
  
  Schedulable function prototype is defined as : void foo(TaskHandle). 
  
  
  Schedulable items can be functions and classes containing these functions. 
  Modules can be added to schedulable items, there is no limitation on the number of modules defining the task/handler
  properties.
  
  The modules are :
      
    - Conditional_M    : Associates a free function as "bool foo()" to each task telling if the function should be
                         executed.
    
    - Coroutine_M      : Implementation of coroutine allowing to yield and loop (Inspired by protothread).
    
    - Coroutine_ctx_M  : Implementation of coroutine allowing to yield, loop and save context.
    
    - CPU_Usage_M      : Measures the CPU usage of tasks.
    
    - Creator_M        : Allows to dynamically allocate objects in fixed size buffer.
    
    - Delay_M          : Allows to delay the execution of a task.
    
    - Interval_M       : Allows to delay and set an execution period of a task.
    
    - Module_Hub_M     : Allows to define several modules per tasks.
    
    - Module_Mix_M     : Allows to define several modules per tasks using mixins.
    
    - Parent_M         : Allows to set a Parent/Child relationship between two tasks, will forbid the
                         deletion of the parent task if the child task is alive. 
                          
    - Priority_M       : Simple priority handling, the highest priority is 1 and the lowest is 255.
                         A kernel is required for this module.
        
    - ProcessQ_M       : Allows to define an order of execution of active tasks.
    
    - Signal_M         : Allows to send data from one task to another.
    
    - Stack_Usage      : Measures the count of bytes written on the stack after the execution of a task.
    
    - Status_M         : Contains the status of the task (Running, Started, Suspended, Locked).
            
    - LinkedList_M     : Automatically updated linked list of chronologically executed active tasks.
    
   
   
  

  uCoSM is divided into three entities which are :
  
    - Modules     : The properties of a schedulable item ( Tasks and TaskHandlers ).
    - TaskHandler : The class scheduler, contains the tasks and their specified modules.
    - Kernel      : The master scheduler, contains the TaskHandlers and their specified modules.
          
        
TaskHandler definition example

      using myTaskModules = ModuleHub_M< Priority_M, Interval_M >;
      const uint8_t maxSimultaneousTaskCount = 1;

      class MyClass : public TaskHandler<MyClass, maxSimultaneousTaskCount, myTaskModules>
      {
        public:
          void schedulableFunction(TaskHandle inHandle){
            // do stuff
          }
      };
    
Kernel definition example

    const uint8_t maxSimultaneousHandlerCount = 1;
    
    Kernel kernel<maxSimultaneousHandlerCount>
