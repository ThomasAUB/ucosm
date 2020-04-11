#pragma once


struct Status_M // 1 byte
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
