#pragma once

// Allow to send data to a specific task
// Data is unaccessible if the owner task is not currently running
template<typename data_t, uint16_t fifo_size>
struct Signal_M
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

