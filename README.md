# uCoSM
Module based cooperative scheduler for microcontroler(Beta)

  uCoSM is a lightweight and modular embedded scheduler designed in C++ (c++14 or above). 
  
  Its features are :
  
    - Dynamically add and remove schedulable items such as function and class.
    - No use of heap memory.
    - Creation of custom task and handler properties called "traits".
    - Fairly low RAM overhead of 10 bytes per tasks. 
    - TaskHandle management.
    - Minimal use of virtual functions.
  
  
  
  Schedulable function prototype is defined as : void foo(void). 
  
  
  Schedulable items can be functions and classes containing these functions. 
  Traits can be added to schedulable items, there is no limitation on the number of traits defining the task/handler
  properties.
  
  The traits are :
  
    - Prio          : Simple priority handling, the higher priority is 1 and the lowest is 255.
    - Status        : Contains the status of the task (Running, Started, Suspended, Locked).
    - StatusNotify  : Callback notification when a specified status has changed. 
    - Delay         : Allows to delay the execution of a task.
    - Periodic      : Allows a task to be called periodically at constant rate.
    - Signal        : Allows to send data from one task to another.
    - Buffer        : Associates a buffer of specified type and size to each tasks of a handler
    - LinkedList    : Automatically updated linked list of chronologically executed tasks.
    - MemPool32     : Allows a fast buffer dynamic allocation of specified size and type, the max buffer
                    count is 32.
    - Parent        : Allows to set a Parent/Child relationship between two tasks, will forbid the
                    deletion of the parent task if the child task is alive. 
    - Coroutine     : Implementation of coroutine allowing non-blocking delay.
    - Coroutine2    : Implementation of coroutine allowing to yield and saving context (Inspired by
                    protothread).
    
  

  uCoSM is divided into three entities which are :
  
    - Traits : the properties of a schedulable item ( Tasks and TaskHandlers ).
    - TaskHandler : the class scheduler, contains the tasks and their specified traits.
    - Kernel : the master scheduler, contains the TaskHandlers and their specified traits.
    
    
Traits definition
    
      Here are some examples of traits definition:
      
        Traits<>
        Traits< Prio >
        Traits< Prio, Delay, LinkedList<0> >
        Traits< MemPool32<std::array<uint8_t, 16>, 32>, Parent, Signal<uint32_t, 8> >
        
        
TaskHandler definition example

      using myTaskTraits = Traits< Prio >;
      const uint8_t maxSimultaneousTaskCount = 1;

      class MyClass : public TaskHandler<MyClass, myTaskTraits, maxSimultaneousTaskCount>
      {
        public:
          void schedulableFunction()
          {
          }
      };
    
Kernel definition example
  
    using myHandlerTraits = Traits<>;
    const uint8_t maxSimultaneousHandlerCount = 1;
    
    Kernel kernel<myHandlerTraits, maxSimultaneousHandlerCount>
