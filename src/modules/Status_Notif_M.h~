#pragma once

// provides a simple status container with notification



enum eNotifStatus
{
	eNotifFirstExe		= 0b00010000,
	eNotifExeStart		= 0b00100000,
	eNotifSuspension	= 0b01000000,
	eNotifDeletion		= 0b10000000,
	eNotifMask			= 0b11110000
};




template<typename callee_t>
struct Status_Notif_M
{
 	
	bool isStatus(uint8_t s){ return ((mStatus&s) == s);}
	
	void setNotifStatus(eNotifStatus s, bool state){
		state ? mStatus|=s : mStatus&=~s;
	}

	bool isRunning(){
		return (mStatus&eRunning);
	}

	bool isStarted(){
		return (mStatus&eStarted);
	}

	bool isPending(){
		return !(mStatus&eStarted);
	}
	
	void init()	{ mStatus = 0; }

	bool isExeReady() const { return true ;}

	bool isDelReady() const { return true ;}

	void makePreExe(){
		
		mStatus |= eRunning;

		if(!(mStatus&eStarted) && (mStatus&eNotifFirstExe)){
			instance->notifyStatusChange (this, eNotifFirstExe);
		}
		
		if(mStatus&eNotifExeStart){
			instance->notifyStatusChange (this, eNotifExeStart);
		}
	}

	void makePreDel(){
		
		if(mStatus&eNotifDeletion){
			instance->notifyStatusChange (this, eNotifDeletion);
		}
		
	}

	void makePostExe(){		
		mStatus &= ~eRunning;
		mStatus |= eStarted;
	}

private:
	
	enum eSystemStatus:uint8_t
	{
		eRunning			= 0b00000001,
		eStarted			= 0b00000010
	};

	static const callee_t *instance;
	
	uint8_t mStatus;
	
};
