#pragma once


struct Conditional_M // 4 bytes
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

