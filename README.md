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
  
### uCoSM is divided into several entities which are :
  
 - ***Modules***     : The properties of a schedulable item.
 - ***TaskObject***      : Contains ITask pointers and their specified modules.
 - ***TaskObjectAllocator***      : Contains ITask instances and their specified modules.
 - ***TaskFunction*** : Contains function pointers and their specified modules.

#### note : 
#### The tasks contained in TaskObject are supposed unique, i.e. only one task per object pointer
#### The tasks contained in TaskFunction are not supposed unique, i.e. multiple task per function pointer is possible



## Modules
      
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
                          
        
 - **ProcessQ_M**       : Defines an execution sequence of active tasks.
    
    
 - **Signal_M**         : Sends data from one task to another.
    
    
 - **Stack_Usage_M**      : Measures the count of bytes written on the stack after the execution of a task.
    
    
 - **Status_M**         : Contains the status of the task (Running, Started, Suspended, Locked).
            
            
 - **LinkedList_M**     : Automatically updated linked list of chronologically executed active tasks.
    
   
   
  
## TaskObject

### Tasks defined as ITask pointers

```cpp
struct MyTask : ITask {
  bool schedule() override final {
    // do stuff...
    return true;
  }
};

MyTask sTask;

// max simultaneous handler count
const uint8_t kHandlerCount = 1;
TaskObject<kHandlerCount> sKernel;

int main(){

  // add handler to kernel
  sKernel.addTask(&sTask);
  
  while(1){
    sKernel.schedule();
  }
  
  return 0;
}
```




## TaskFunction

### Tasks defined as function pointers

```cpp

// task properties or features
using myTaskModules = ModuleMix_M< Interval_M, Conditional_M >;

bool isReady() {
  return true;
}

// max simultaneous task count
const uint8_t kTaskCount = 1;

class MyTaskClass : public TaskFunction<MyTaskClass, kTaskCount, myTaskModules>
{

  public:
  
    MyTaskClass() : mDelay(true), mExeCount(0) {
      
      // create task handle
      TaskHandle h;
      
      // create task execution
      if(this->createTask(&MyTaskClass::myTaskFunction, &h)) {
        h->setPeriod(5);
        h->setCondition(isReady);
      }
    }
  
  private:
  
    void myTaskFunction(TaskHandle h){
    
      // do stuff
      if(mDelay) {
        h->setDelay(50);
      }
      
      mDelay = !mDelay;
      
      if(mExeCount++ == 200) {
        this->deleteTask(h);
      }
      
    }
    
    bool mDelay;
    uint8_t mExeCount;
};

MyTaskClass sTask;

// max simultaneous handler count
const uint8_t kHandlerCount = 1;
TaskObject<kHandlerCount> sKernel;


int main() {

  sKernel.addTask(&sTask);

  while(1) {
    sKernel.schedule();
  }
  return 0;
}
```


## Custom module

#### To create your own module, the simplest approach is to start from void_M.
#### It's the minimal mandatory definition.
#### From here, you can add functions and members you need for your program.
#### An instance of module is associtated to every tasks.

```cpp
struct void_M {

  // will be called each time the task is created or added
  void init() {}

  // the result of this call will tell if the task must be executed or not
  bool isExeReady() const { return true; }

  // the result of this call will tell if the deletion of the task is allowed or not
  bool isDelReady() const { return true; }

  // called right before every executions of the task
  void makePreExe() {}

  // called right before the deletion of the task
  void makePreDel() {}

  // called right after every executions of the task
  void makePostExe() {}

};
```