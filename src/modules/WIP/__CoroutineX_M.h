#pragma once


// Be careful with this one!
// It should not be employed in an infinite loop
// It will mess up thisTaskHandle()
// You'll have to store the current task value
struct CoroutineX_M
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
