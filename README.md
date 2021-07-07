[![Build Status](https://travis-ci.com/ThomasAUB/uCoSM.svg?branch=master)](https://travis-ci.com/ThomasAUB/uCoSM)

# uCoSM
Module based cooperative scheduler for microcontroler

  uCoSM is a lightweight and modular C++ embedded dynamic scheduler (c++14 or above). 
  
## Its features are :
  
 - Dynamically add and remove schedulable items such as function and class.
 - No use of heap memory.
 - Creation of custom task and handler properties called "modules".
 - TaskHandle management.
 - Minimal use of virtual functions.
   
  
  Schedulable items can be functions or objects. 
  Modules can be added to those in order to add features.
  
## The modules are :
      
 - **Conditional_M** : Associates a free function as "bool foo()" to each task telling if the function should be executed.
    
 - **Coroutine_M** : Implementation of coroutine allowing to yield and loop (Inspired by protothread).
    
 - **Coroutine_ctx_M** : Implementation of coroutine allowing to yield, loop and save context.
    
    
 - **CPU_Usage_M** : Measures the CPU usage of tasks.
    
    
 - **Creator_M**        : Dynamically allocates an object in a shared fixed size buffer.
    
    
 - **Delay_M**          : Delays the execution of a task.
    
    
 - **Interval_M**       : Delays and set an execution period of a task.
    
    
 - **Module_Hub_M**     : Defines several modules per tasks.
    
    
 - **Module_Mix_M**     : Defines several modules per tasks using mixins.
    
    
 - **Parent_M**         : Sets a Parent/Child relationship between two tasks, will forbid the deletion of the parent task if the child task is alive. 
                          
 - **Priority_M**       : Simple priority handling, the highest priority is 1 and the lowest is 255. A kernel is required for this module.
        
 - **ProcessQ_M**       : Defines an execution sequence of active tasks.
    
    
 - **Signal_M**         : Sends data from one task to another.
    
    
 - **Stack_Usage_M**      : Measures the count of bytes written on the stack after the execution of a task.
    
    
 - **Status_M**         : Contains the status of the task (Running, Started, Suspended, Locked).
            
            
 - **LinkedList_M**     : Automatically updated linked list of chronologically executed active tasks.
    
   
   
  

### uCoSM is divided into three entities which are :
  
 - ***Modules***     : The properties of a schedulable item.
 - ***TaskFunction*** : Contains function pointers and their specified modules.
 - ***TaskObject***      : Contains iTask pointers and their specified modules.
          
        
### TaskFunction definition example
```cpp
// task properties or features
using myTaskModules = ModuleHub_M< Priority_M, Interval_M >;

// max simultaneous task count
const uint8_t kTaskCount = 1;

class MyClass : public TaskFunction<MyClass, kTaskCount, myTaskModules>
{

  public:
  
    MyClass(){
      // create task execution
      this->createTask(&MyClass::myTaskFunction);
    }
  
  private:
  
    void myTaskFunction(TaskHandle inHandle){
      // do stuff
    }
    
};
```
### TaskObject definition example
```cpp
// max simultaneous handler count
const uint8_t kHandlerCount = 1;

TaskObject<kHandlerCount> sKernel;

MyClass sHandler;

int main(){

  // add handler to kernel
  sKernel.addTask(&sHandler);
  
  while(1){
    sKernel.schedule();
  }
  
  return 0;
}
```
