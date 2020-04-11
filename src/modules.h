/*
 * Copyright (C) 2020 Thomas AUBERT <aubert.thms@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Thomas AUBERT'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * uCosmDev IS PROVIDED BY Thomas AUBERT ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Thomas AUBERT OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#pragma once

#include <type_traits>
#include "uscosm-sys-data.h"
#include "utils.h"

//-------------------------------  Task Traits  -------------------------------

// µCoSm - Cooperative Scheduler Module


/*  Task modules :
 * 
 *  must contain a set of funtions :
 * 
 *	  - template<typename T> void init(T *t)
 * 
 *	  - bool isExeReady const ()
 *	  - bool isDelReady const ()
 *
 *	  - void makePreExe()
 *	  - void makePreDel()
 *	  - void makePostExe()
 *	  	  
 * 
 */




#include <tuple>



template<class ...ModuleCollection> 
class Modules
{

	using items_t = std::tuple<ModuleCollection...>;
	items_t itemModules;

	
public:
	
	Modules()
	{}

	template<typename T>
	auto get()
	{
		return std::get<T>(itemModules);
	}

	template<typename T, size_t I = 0>
	void init(T *t)
	{
		std::get<I>(itemModules).init(t);
		if constexpr(I+1 != std::tuple_size<items_t>::value)
		    init<T, I+1>(t);
	}

	template<typename T, size_t I = 0>
	bool isExeReady(T *t)
	{
		if constexpr (I+1 != std::tuple_size<items_t>::value)
			return ( isExeReady<T, I+1>(t) && std::get<I>(itemModules).isExeReady(t) );
		else 
			return std::get<I>(itemModules).isExeReady(t);
	}
	
	template<typename T, size_t I = 0>
	bool isDelReady(T *t) {
		if constexpr (I+1 != std::tuple_size<items_t>::value)
			return ( isDelReady<T, I+1>(t) && std::get<I>(itemModules).isDelReady(t) );
		else 
			return std::get<I>(itemModules).isDelReady(t);
	}

	template<typename T, size_t I = 0>
	void makePreExe(T *t) {
		std::get<I>(itemModules).makePreExe(t);
		if constexpr(I+1 != std::tuple_size<items_t>::value)
		    makePreExe<T, I+1>(t);
	}

	template<typename T, size_t I = 0>
	void makePostExe(T *t) {
		std::get<I>(itemModules).makePostExe(t);
		if constexpr(I+1 != std::tuple_size<items_t>::value)
		    makePostExe<T, I+1>(t);
	}
	
	template<typename T, size_t I = 0>
	void makePreDel(T *t) {
		std::get<I>(itemModules).makePreDel(t);
		if constexpr(I+1 != std::tuple_size<items_t>::value)
		    makePreDel<T, I+1>(t);
	}
};




struct no_module
{
	template<typename T>
	void init(T *t){}
	template<typename T>
    bool isExeReady(T *t) const {return true;}
	template<typename T>
	bool isDelReady(T *t) const {return true;}
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}
};








namespace ucosm_modules
{





// Allows to add basic task priority management :
// the priority goes from 1 to 255
// where 1 is the highest priority i.e. it will be executed on every mainloop cycles
// and 255 is the lowest, it will be executed once every 255 mainloop cycles

struct Prio // 1 byte
{

	void setPriority(const uint8_t inPrio)
	{
		// priority can't be inferior to 1
		mPriority = (inPrio)?inPrio:1; 
	}

	template<typename T>
	void init(T *t)
	{
		mPriority = 1;
	}

	template<typename T>
    bool isExeReady(T *t)
	{   
		if(SysKernelData::sCnt)
		{
			return (!(SysKernelData::sCnt%mPriority));
		}else{
			return (!((SysKernelData::sCnt+1)%mPriority));
		}
	}

	template<typename T>
	bool isDelReady(T *t) const {return true;}
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}
	
private:

    uint8_t mPriority;
};












struct Status // 1 byte
{
 
	enum eStatus:uint8_t
	{
		eSuspended			= 0b00000100,
		eLocked				= 0b00001000,
		eStatusMask			= 0b00001111
	};

		
	bool isStatus(uint8_t s){ return ((mStatus&s) == s);}
	
	void setStatus(uint8_t s, bool state)
	{
		// task is locked : cancel operation
		if(isStatus(eLocked) && s!=eLocked){ return;}
		
		state ? mStatus|=s : mStatus&=~s;
	}

	void setStatus(eStatus s, bool state)
	{
		setStatus(static_cast<uint8_t>(s), state);
	}

	bool isRunning()
	{
		return (mStatus&eRunning);
	}

	bool isStarted()
	{
		return (mStatus&eStarted);
	}
	
	template<typename T>
	void init(T *t)	{ mStatus = 0; }
	template<typename T>
	bool isExeReady(T *t) const { return !(mStatus&eSuspended) ;}
	template<typename T>
	bool isDelReady(T *t) const { return !(mStatus&eLocked);}
	template<typename T>
	void makePreExe(T *t){  mStatus |= eRunning; }
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t)
	{
		mStatus &= ~eRunning; 
		mStatus |= eStarted;
	}
	
	enum eSystemStatus:uint8_t
	{
		eRunning			= 0b00000001,
		eStarted			= 0b00000010
	};
	
	uint8_t mStatus;
	
};






template<typename Callee_t>
struct StatusNotify : private Status // 1 byte
{
 
	enum eNotifyStatus
	{
		eNotifyStarted		= Status::eStarted	<<3,	// 0b00010000
		eNotifySuspended	= Status::eSuspended<<3,	// 0b00100000
		eNotifyLocked		= Status::eLocked	<<3,	// 0b01000000
		eNotifyDeleted		= 0b10000000,				// 0b10000000
		eNotifyMask			= ~Status::eStatusMask
    };
	
	bool isStatus(uint8_t s){ return ((mStatus&s) == s); }
		
	void setStatus(uint8_t s, bool state)
	{
		if(isStatus(Status::eLocked) && s!=Status::eLocked){ return;}

		uint8_t prevNotifStatus = (mStatus&Status::eStatusMask)<<3;
		
		state ? mStatus|=s : mStatus&=~s;

		// is the new status notifiable
		uint8_t newNotifStatus = ((s&Status::eStatusMask)<<3)&(mStatus&eNotifyMask);
		
		if(newNotifStatus)// status notifiable: notify callee
		{
			Callee_t::notifyStatusChange(prevNotifStatus, newNotifStatus);
		}
	}
	
	void setStatus(Status::eStatus s, bool state)
	{
		setStatus(static_cast<uint8_t>(s), state);
	}
	

	template<typename T>
	void init(T *t)	{ mStatus = 0; }
	template<typename T>
	bool isExeReady(T *t) const { return !(mStatus&Status::eSuspended) ;}
	template<typename T>
	bool isDelReady(T *t) const { return !(mStatus&Status::eLocked);}
	template<typename T>
	void makePreExe(T *t)
	{  
		setStatus(Status::eRunning, true); 
	}
	template<typename T>
	void makePostExe(T *t)
	{
		setStatus(Status::eRunning, false); 
		setStatus(Status::eStarted, true);
	}
	template<typename T>
	void makePreDel(T *t)
	{
		if( isStatus(eNotifyDeleted) )
		{
			Callee_t::notifyStatusChange(mStatus, eNotifyDeleted);
		}
		mStatus = 0;
	}
	
};











struct Delay // 4 bytes
{

	void setDelay(tick_t inDelay)
	{
		mExecution_time_stamp = SysKernelData::sGetTick()+inDelay; 
	}
	
	tick_t getDelay()
	{	
		if(mExecution_time_stamp > SysKernelData::sGetTick()){
			return mExecution_time_stamp - SysKernelData::sGetTick(); 
		}else{
			return 0;
		}
	}


	template<typename T>
	void init(T *t)
	{
		mExecution_time_stamp = SysKernelData::sGetTick();
	}
	template<typename T>
    bool isExeReady(T *t) const 
	{
		return (SysKernelData::sGetTick() >= mExecution_time_stamp);
	}
	template<typename T>
	bool isDelReady(T *t) const { return true; }
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}	
	
private:
	
	tick_t mExecution_time_stamp;
	
};






// periodic call of the function, guarantees a constant average execution rate
struct Periodic // 6 bytes
{

	using period_t = uint16_t;

	
	void setPeriod(period_t inPeriod)
	{
		mPeriod = inPeriod;
		mExecution_time_stamp += mPeriod;
	}

	period_t getPeriod()
	{
		return mPeriod;
	}

	void setDelay(tick_t inDelay)
	{	
		mExecution_time_stamp = SysKernelData::sGetTick()+inDelay; 
	}
	
	tick_t getDelay()
	{	if(mExecution_time_stamp > SysKernelData::sGetTick()){
			return mExecution_time_stamp - SysKernelData::sGetTick(); 
		}else{
			return 0;
		}
	}

	template<typename T>
	void init(T *t)
	{
		mExecution_time_stamp = SysKernelData::sGetTick();
	}
	template<typename T>
	bool isExeReady(T *t) const {
		return (SysKernelData::sGetTick() >= mExecution_time_stamp);
	}
	template<typename T>
	bool isDelReady(T *t) { return true; }
	template<typename T>
	void makePreExe(T *t)
	{
		mExecution_time_stamp += mPeriod;
	}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}
		
private:
	
	tick_t mExecution_time_stamp;
	period_t mPeriod;
};







struct Conditional // 4 bytes
{	
	void setCondition(bool (*inCondition)())
	{
		mCondition = inCondition;
	}
	
	template<typename T>
	void init(T *t)
	{
		mCondition = nullptr;
	}
	template<typename T>
    bool isExeReady(T *t)
	{
		if(!mCondition){
			return true;
		}
		return mCondition();
	}
	template<typename T>
	bool isDelReady(T *t) const { return true; }
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}	
	
private:

	bool (*mCondition)();
	
};









// Allow to send data to a specific task
// Data is unaccessible if the owner task is not currently running
template<typename data_t, uint16_t fifo_size>
struct Signal
{
	
	bool send(Signal *inReceiver, data_t inData)
	{
		if(!inReceiver){return false;}
		return (inReceiver->mRxData.push(inData));
	}

	data_t receive()
	{
		if(!reinterpret_cast<Status *>(this)->isRunning()){
			return data_t();
		}

		return mRxData.pop();
	}

	bool hasData()
	{
		return !mRxData.isEmpty();
	}

	template<typename T>
	void init(T *t)
	{ 
		static_assert(std::is_base_of<Status, T>::value, "Signal must implement Status");
	}
	template<typename T>
	bool isExeReady(T *t) { return true; }
	template<typename T>
	bool isDelReady(T *t) { return mRxData.isEmpty(); }
	template<typename T>
	void makePreExe(T *t) {}
	template<typename T>
	void makePreDel(T *t) {}
	template<typename T>
	void makePostExe(T *t){}

private:
	
	Fifo<data_t, fifo_size> mRxData;
};












// contains a buffer of the specified type and size
template<typename buffer_t, uint16_t size> 
struct Buffer
{

	void setData(buffer_t *inData, uint16_t inByteSize)
	{
		if(inByteSize > size*sizeof(buffer_t)) { return; }
		memcpy(mBuffer, inData, inByteSize);
	}

	void setDataAt(uint16_t inIdx, buffer_t inData)
	{
		if(inIdx >= size) { return; }
		mBuffer[inIdx] = inData;
	}
	
	buffer_t getData(uint16_t inIdx)
	{
		if(inIdx >= size) { return 0; }
		return mBuffer[inIdx];
	}
	
	buffer_t *getBuffer()
	{
		return mBuffer;
	}

	template<typename T>
	void init(T *t) {}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}

	buffer_t mBuffer[size];
};

























struct ListItem
{
	ListItem* mPrev;
	ListItem* mNext;
};



// automatically updated linked list of tasks by chronology of execution
template<int listIndex>
struct LinkedList : public ListItem // 9 bytes
{

	ListItem *getNext() { return mNext; }
	ListItem *getPrev() { return mPrev; }

	template<typename T>
	void init(T *t)
	{   
		//static_assert(std::is_base_of<Status, T>::value, "LinkedList must implement Status");
		//t->get<Status>().setStatus(0);
		mPrev = mNext = nullptr;
	}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 
	template<typename T>
	void makePreExe(T *t)
	{
		if(reinterpret_cast<Status *>(this)->isStarted()){return;}

		if(sTopHandle)
		{
			sTopHandle->mNext = this;
			mPrev = sTopHandle;
		}
		sTopHandle = this;
	}
	template<typename T>
	void makePreDel(T *t)
	{
		if(sTopHandle == this)
		{
			if(mPrev){
				sTopHandle = mPrev;
			}else{
				sTopHandle = nullptr;
			}
		}
		if(mPrev && mNext)
		{
			mPrev->mNext = mNext;
			mNext->mPrev = mPrev;
		}else if(mPrev){
			mPrev->mNext = nullptr;
		}else if(mNext){
			mNext->mPrev = nullptr;
		}
	}
	template<typename T>
	void makePostExe(T *t){}

private:

	static ListItem *sTopHandle;
	
};

template<int listIndex>
ListItem *LinkedList<listIndex>::sTopHandle = nullptr;










































// Fixed size dynamic memory allocation
// features :
//  - Allows to allocate and release a buffer of sizeof(elem_t) bytes
//  - Forbids task deletion if a buffer is allocated to avoid memory leakage
//  - The number of buffer per task types has a maximum value of 32
//	- If auto_release is "true", the module will automatically release allocated memory on deletion 

template<typename elem_t, uint16_t elem_count, bool auto_release = true>
struct MemPool32
{
	static_assert(elem_count <= 32, "size of pool must not exceed 32");
	
	static_assert( (sizeof(elem_t) * sizeof(elem_count) ) > 4, 
	"Suboptimal implementation : Pool's size inferior to overhead's");

	template<typename T>
	T *allocate()
	{
		static_assert(sizeof(T) <= sizeof(elem_t), "Allocation error");
		elem_t *e = allocate();
		if(e == nullptr){ return nullptr; }
		return reinterpret_cast<T *>(e);
	}
		
	// could use "placement new" here
	elem_t *allocate()
	{
		// pool is full
		if(mMemoryMap == (1<<elem_count)-1){ return nullptr; }

		// task already has allocated memory
		if(mAllocIndex){ return nullptr; }

		
		uint8_t i=0;
		
		do{
			if(!mMemoryMap&(1<<i)) // slot free
			{
				mMemoryMap |= (1<<i); // take slot

				mAllocIndex = i; // stores the index for fast deletion
				
				// set task alloc active with a boolean,
				// allows to check allocation in case index is 0
				mAllocIndex |= kAllocBoolMask;
				
				return &mElems[i];
			}
		}while(++i<elem_count);
		
		// allocation error : should not happen
		return nullptr;
	}
		
	bool release(){

		// pool is empty, nothing to free
		if(!mMemoryMap){ return false; }

		// task has no allocated memory
		if(!mAllocIndex){ return false; }

		// remove boolean alloc state
		mAllocIndex &= ~kAllocBoolMask;

		// security check : verify that the memory has been allocated
		// critical error : map and index does not coincide
		if(!(mMemoryMap&(1<<mAllocIndex))){ return false; }

		// release memory
		mMemoryMap &= ~(1<<mAllocIndex);

		// delete index
		mAllocIndex = 0;
		return true;

	}

	template<typename T>
	T *getMemory()
	{
		static_assert(sizeof(T) <= sizeof(elem_t), "Allocation error");
		elem_t *e = getMemory();
		if(e == nullptr){ return nullptr; }
		return reinterpret_cast<T *>(e);
	}

	elem_t *getMemory()
	{
		if(!mAllocIndex){ return nullptr; }
		return &mElems[mAllocIndex&(~kAllocBoolMask)];
	}

	template<typename T>
	void init(T *t) { mAllocIndex = 0; }
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	
	// decide if deletion forbidden if allocated memory or auto release?
	template<typename T>
	bool isDelReady(T *t) 
	{ 
		if(mAllocIndex) // memory is allocated
		{
			if constexpr (auto_release)
				return release();
			else
				return false;
		}
		return true;
	} 
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}
	
private:

	uint8_t mAllocIndex;
	
	static elem_t mElems[elem_count];
	static uint32_t mMemoryMap;
	static const uint8_t kAllocBoolMask = 0b10000000;

};

template <typename elem_t, uint16_t elem_count, bool auto_release>
elem_t MemPool32<elem_t, elem_count, auto_release>::mElems[elem_count];

template <typename elem_t, uint16_t elem_count, bool auto_release>
uint32_t MemPool32<elem_t, elem_count, auto_release>::mMemoryMap = 0;













// allows to set a Parent/Child relation between two tasks,
// it forbids the deletion of the parent task if the child is alive
struct Parent
{

	void setChild(Parent *inChild)
	{
		inChild->mParent = this;
		mChild = inChild;
		mIsParent = true;
	}

	template<typename T>
	void init(T *t)
	{ 
		mIsParent = false;
		mChild = nullptr; 
	}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t)
	{
		if(!mIsParent) { return true; }
		
		if(!mChild)	{ return true; }
	
		return false; 
	} 
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t)
	{
		if(!mIsParent && mParent)
		{
			mParent->mChild = nullptr;
		}
	}
	template<typename T>
	void makePostExe(T *t){}

private:

	union{
		Parent *mChild;
		Parent *mParent;
	};
	
	bool mIsParent;
};









// Be careful with this one!
// It should not be employed in an infinite loop
// It will mess up thisTaskHandle()
// You'll have to store the current task value
struct Coroutine
{

	void waitFor(tick_t inDuration)
	{
		if(SysKernelData::sMaster)
		{	
			SysKernelData::sMaster->schedule(inDuration);	
		}
	}
						
	template<typename T>
	void init(T *t)
	{ 
		static_assert(std::is_base_of<Status, T>::value, "Coroutine must implement Status");
	}
	template<typename T>
	bool isExeReady(T *t) 
	{ 
		return !(reinterpret_cast<Status *>(this)->isRunning());
	}
	template<typename T>
	bool isDelReady(T *t){return true;}
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}

private:
	
};






// macro based coroutine
// inspired by protothread

template<uint16_t max_context_size> 
struct Coroutine2
{

#define CTX_START				struct Ctx_def{

#define CTX_END(label)			};Ctx_def *label = thisTaskHandle()->getContext<Ctx_def>();

#define CR_START				switch(thisTaskHandle()->mLine){case 0:

#define CR_YIELD				thisTaskHandle()->mLine = __LINE__;return;case  __LINE__  :

#define CR_WAIT_UNTIL(cond)		thisTaskHandle()->mLine = __LINE__;case __LINE__ :  if(!(cond)){return;}

#define CR_WAIT_FOR(delay)		thisTaskHandle()->setDelay(delay);CR_YIELD // set delay and YIELD

#define CR_LOOP(cond)			for(;(cond);CR_YIELD)

#define CR_RESET				thisTaskHandle()->mLine = 0;return;
	
#define CR_END         			}deleteTask(thisTaskHandle());


	
	template<typename T>
	T *getContext()
	{
		return reinterpret_cast<T *>(mContext);
	}

	uint16_t mLine;

	template<typename T>
	void init(T *t) { mLine = 0;}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}

private:
	
	uint8_t mContext[max_context_size];
};



template<uint16_t max_stack_usage, uint8_t data_filter_coefficient = 3> 
struct TaskAnalizer
{

	static_assert(max_stack_usage >= sizeof(uint32_t), "max_stack_usage must >= 4");
	
	using cycle_t = uint16_t;

	enum eAnalizeType{
		eEnd,
		eStackUsage,
		eExecTime,
		eCPUMeasure,
		eTypeCount
	};

	enum eResult{
		eNone,
		ePassed,
		eErrorMemoryLeak,
		eUserTerminated
	};

	void start(cycle_t inExeCount)
	{
		if(!isAnalizerAvailable()){
			return;
		}
		sExeCount = inExeCount;
		sCurAnalyzer = this;
		sType = eStackUsage;
		sResult = eNone;
		sTotalTime = getTime();
	}

	void stop()
	{
		terminate(eUserTerminated);
	}

	bool isAnalizerAvailable()
	{
		return (sCurAnalyzer != nullptr);
	}

	bool isAnalizerRunning()
	{
		return (sCurAnalyzer == this);
	}

	void setTimeBase(tick_t (*inGetTick)())
	{
		sGetTick = inGetTick;
	}
	
	template<typename T>
	void init(T *t) {}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) { return !isAnalizerRunning(); } 
	template<typename T>
	void makePreExe(T *t)
	{
		if(!isAnalizerRunning())
		{
			return;
		}

		switch(sType){
			case eStackUsage:
			{
				const auto size=max_stack_usage/sizeof(uint32_t);
				volatile uint32_t s[size];
				sSp = s;
				uint16_t i=0;
				while(i < size){
					s[i] = kStackPattern;
				}
			}
				break;
			case eExecTime:
				 sCurrExeTime = getTime();
				break;
			case eCPUMeasure:
				break;
		}
		
	}
	template<typename T>
	void makePostExe(T *t)
	{
		if(!isAnalizerRunning())
		{
			return;
		}

		switch(sType){
			case eStackUsage:
			{
				uint32_t dummy;
				if(sSp != &dummy){
					//stack overflow or memory leakage	
					terminate(eErrorMemoryLeak);
				}

				const auto size=max_stack_usage/sizeof(uint32_t);
				uint32_t s[size];
				uint16_t i=0;
				uint16_t currStackUsage = 0;
				while(i < size){
					if(s[i] == kStackPattern){
						// stack usage
						currStackUsage = i*sizeof(uint32_t);
						break;
					}
					i++;
				}
			
				sAverageStackUsage = smooth(sAverageStackUsage, currStackUsage);
				if( currStackUsage > sMaxStackUsage){
				 	sMaxStackUsage =  currStackUsage;
				}
				}
				break;
			case eExecTime:
				sCurrExeTime = getTime() - sCurrExeTime;
				sAverageExeTime  = smooth(sAverageExeTime, sCurrExeTime);
				if(sCurrExeTime > sMaxExeTime){
					sMaxExeTime =  sCurrExeTime;
				}
				break;
			case eCPUMeasure:
				break;
		}

		if(sExeCounter++ == sExeCount)
		{
			sType = (sType+1)%eTypeCount;
			sExeCounter = 0;
			if(sType == eEnd)
			{	// terminate analize
				terminate(ePassed);
			}			
		}
	}

	template<typename T>
	void makePreDel(T *t){}

	// stack
	static uint16_t sAverageStackUsage;
	static uint16_t sMaxStackUsage;

	// exe time
	static tick_t sAverageExeTime;
	static tick_t sMaxExeTime;

	// global
	static tick_t sTotalTime;

	static eResult sResult;
	
private:

	void terminate(eResult inResult)
	{
		sCurAnalyzer = nullptr;
		sTotalTime = getTime() - sTotalTime;
	}
	
	template<typename T>
	T smooth(T inPrev, T inNew)
	{
		(inPrev*(1<<(data_filter_coefficient-1)) + inNew)/(1<<data_filter_coefficient);
	}

	tick_t getTime()
	{
		return sGetTick();
	}
	
	static TaskAnalizer *sCurAnalyzer;

	static cycle_t sExeCounter;
	static cycle_t sExeCount;

	static eAnalizeType sType;

	// stack usage
	static uint32_t *sSp;
	

	// exe time
	static tick_t sCurrExeTime;
	static tick_t (*sGetTick)();
	
	static const uint32_t kStackPattern = 0xAACCBBDD;
};




} // end of task_traits namespace 











