#pragma once




// allows to set a Parent/Child relation between two tasks,
// it forbids the deletion of the parent task if the child is alive
struct Parent_M
{

	void setChild(Parent_M *inChild)
	{
		inChild->mParent = this;
		mChild = inChild;
		mIsParent = true;
	}

	void init()
	{ 
		mIsParent = false;
		mChild = nullptr; 
	}

	bool isExeReady() const { return true; }

	bool isDelReady()
	{
		if(!mIsParent) { return true; }
		
		if(!mChild)	{ return true; }
	
		return false; 
	} 

	void makePreExe(){}

	void makePreDel()
	{
		if(!mIsParent && mParent)
		{
			mParent->mChild = nullptr;
		}
	}

	void makePostExe(){}

private:

	union{
		Parent_M *mChild;
		Parent_M *mParent;
	};
	
	bool mIsParent;
};

