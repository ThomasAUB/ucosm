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
	
	void setStatus(eStatus s, bool state)
	{
		// task is locked : cancel operation
		if(isStatus(eLocked) && s!=eLocked){ return;}
		state ? mStatus|=s : mStatus&=~s;
	}

	bool isRunning()
	{
		return (mStatus&eRunning);
	}

	bool isStarted()
	{
		return (mStatus&eStarted);
	}

	bool isPending()
	{
		return !(mStatus&eStarted);
	}
	
	void init()	{ mStatus = 0; }

	bool isExeReady() const { return !(mStatus&eSuspended) ;}

	bool isDelReady() const { return !(mStatus&eLocked);}

	void makePreExe(){  mStatus |= eRunning; }

	void makePreDel(){}

	void makePostExe()
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
