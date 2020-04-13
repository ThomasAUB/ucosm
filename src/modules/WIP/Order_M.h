#pragma once


struct Order_M
{

	void setPrevious(Order_M *inPrev){
		mPrev = inPrev;
	}

	void init(){
		mPrev = nullptr;
		mIsStarted = false;
	}

    	bool isExeReady() const {
		if(mPrev){
			return mPrev->mIsStarted;
		}else{
			return true;
		}
	}

	bool isDelReady() const {return true;}

	void makePreExe(){}

	void makePreDel(){}

	void makePostExe(){
		mIsStarted = true;	
	}

private:

	Order_M *mPrev;
	bool mIsStarted;

};




/*



template<uint8_t rank_index = 0>
struct Order_M
{

	void setRank(uint8_t inRank){
		mRank = inRank;
	}

	void init(){
		mRank = 0;
	}

    	bool isExeReady() const {
		return (mRank >= sLastRankedExe);
	}

	bool isDelReady() const {return true;}

	void makePreExe(){
		sLastRankedExe = mRank;
	}

	void makePreDel(){
		
	}

	void makePostExe(){}

private:

	uint8_t mRank;

	static uint8_t sLastRankedExe;
};

template<uint8_t rank_index>
uint8_t Order_M<rank_index>::sLastRankedExe = 0;

*/
