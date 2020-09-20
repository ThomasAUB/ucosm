#pragma once


struct ProcessCounter_M
{

	void init();

    bool isExeReady() const {return true;}

	bool isDelReady() const {return true;}

	void makePreExe(){}

	void makePreDel();

	void makePostExe(){}

	size_t getCount();

private:

	static size_t mActiveCount;
};
