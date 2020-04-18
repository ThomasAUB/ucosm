#pragma once



struct Suspendable_M
{

	void setSuspend(bool state){
		if(state && mSuspend < 255){
			mSuspend++;
		}else if(!state && mSuspend){
			mSuspend--;
		}
	}

	void init(){
		mSuspend = 0;
	}

	bool isExeReady() const {
		return (mSuspend == 0);
	}

	bool isDelReady() const {return true;}

	void makePreExe(){}

	void makePreDel(){}

	void makePostExe(){}

private :

	uint8_t mSuspend;
};
