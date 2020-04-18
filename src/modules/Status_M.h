#pragma once

// provides a simple status container


struct Status_M
{
 	
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
	}

	void makePreDel(){}

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
	
	uint8_t mStatus;
	
};


