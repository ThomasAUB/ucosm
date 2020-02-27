# uCoSM
Module based cooperative scheduler for microcontroler(Beta)

  uCoSM is an embedded scheduler designed in C++. Its purpose is to be able to dynamically add and remove schedulable items.
  
  
  Schedulable items can be functions and classes containing these functions. 
  Traits can be added to schedulable items, there is no limitation on the number of traits defining the task/handler
  properties.
  
  The traits are :
  
    - Prio : Simple priority handling, the higher priority is 1 and the lowest is 255.
    - Status : Contains the status of the task (Running, Started, Suspended, Locked).
    - StatusNotify : Callback notification when a specified status has changed. 
    - Delay : Allows to delay the execution of a task.
    - Periodic : Allows a task to be called periodically at constant rate.
    - Signal : Allows to send data from one task to another.
    - Buffer : Associates a buffer of specified type and size to each tasks of a handler
    - LinkedList : Automatically updated linked list of chronologically executed tasks.
    - MemPool32 : Allows a fast buffer dynamic allocation of specified size and type, the max buffer count is 32.
    - Parent :  Allows to set a Parent/Child relationship between two tasks, will forbid the deletion of the parent task 
                if the child task is alive. 
    - Coroutine : Implementation of coroutine allowing non-blocking delay.
    - Coroutine2 : Implementation of coroutine allowing to yield and saving context (Inspired by protothread).
    
  

  uCoSM is divided into three entities which are :
  
    - Traits : the properties of a schedulable item ( Tasks and TaskHandlers ).
    - TaskHandler : the class scheduler, contains the tasks and their specified traits.
    - Kernel : the master scheduler, contains the TaskHandlers and their specified traits.
    
    
Traits definition
    
      Here are some examples of traits definition:
      
        Traits< Prio >
        Traits< Prio, Delay, LinkedList<0> >
        Traits<MemPool32< std::array<uint8_t, 16>, 32>, Parent, Signal<uint32_t, 8> >
    
