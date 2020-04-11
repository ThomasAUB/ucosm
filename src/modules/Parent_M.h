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
		Parent_M *mChild;
		Parent_M *mParent;
	};
	
	bool mIsParent;
};

