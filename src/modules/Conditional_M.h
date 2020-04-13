#pragma once


struct Conditional_M // 4 bytes
{	
	void setCondition(bool (*inCondition)())
	{
		mCondition = inCondition;
	}
	
	void init()
	{
		mCondition = nullptr;
	}

    	bool isExeReady()
	{
		if(!mCondition)	return true;
		return mCondition();
	}

	bool isDelReady() const { return true; }

	void makePreExe(){}

	void makePreDel(){}

	void makePostExe(){}	
	
private:

	bool (*mCondition)();
	
};

