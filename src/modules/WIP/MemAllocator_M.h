#pragma once


template<typename allocator_t>
struct MemAllocator_M
{
	
	template<typename T>
	bool newBlock(T *t)
	{
		// only one block allowed per tasks
		if(allocator_t::isAllocated2(mMemID)){
			return false;
		}
		return allocator_t::newBlock(t, mMemID);
	}

	bool deleteBlock()
	{
		return allocator_t::deleteBlock(mMemID);
	}

	template<typename T>
	bool getBlock(T *t)
	{
		return allocator_t::getBlock(t, mMemID);
	}

	void setAutoRelease(bool inState){
		mAutoRelease = inState;
	}

	template<typename T>
	void init(T *t) {
		mAutoRelease = true;
	}

	template<typename T>
	bool isExeReady(T *t) const { return true; }
	
	template<typename T>
	bool isDelReady(T *t)
	{
		if(allocator_t::isAllocated(mMemID)) // memory is allocated
		{
			if (mAutoRelease)
				return deleteBlock();
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
	
	allocator_t::blockID_t mMemID;

	bool mAutoRelease;
	
};



