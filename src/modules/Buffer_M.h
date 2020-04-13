#pragma once


// contains a buffer of the specified type and size
template<typename buffer_t, size_t size> 
struct Buffer_M
{

	void setData(buffer_t *inData, size_t inByteSize)
	{
		if(inByteSize > size*sizeof(buffer_t)) { return; }
		memcpy(mBuffer, inData, inByteSize);
	}

	void setDataAt(size_t inIdx, buffer_t inData)
	{
		if(inIdx >= size) { return; }
		mBuffer[inIdx] = inData;
	}
	
	buffer_t& getDataAt(size_t inIdx)
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

private:

	buffer_t mBuffer[size];
};

