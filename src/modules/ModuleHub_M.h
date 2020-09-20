#pragma once


#include <tuple>

// provides a module container







template<class ...ModuleCollection> 
struct ModuleHub_M : public ModuleCollection...
{

	ModuleHub_M()
	{}

	void init()
	{
		uint8_t d[] = {(uint8_t)0, (ModuleCollection::init(), (uint8_t)0)...};
		static_cast<void>(d); // avoid warning for unused variable
	}

	bool isExeReady()
	{
		bool ready[] = {
			true, (ModuleCollection::isExeReady())...
		};
		for(size_t i=0 ; i<sizeof(ready) ; i++)
		{
			if(!ready[i]){ return false; }
		}
		return true;
	}
	
	bool isDelReady()
	{
		bool ready[] = {
			true, (ModuleCollection::isDelReady())...
		};
		for(size_t i=0 ; i<sizeof(ready) ; i++)
		{
			if(!ready[i]){ return false; }
		}
		return true;
	}

	void makePreExe()
	{
		uint8_t d[] = {(uint8_t)0, (ModuleCollection::makePreExe(), (uint8_t)0)...};
		static_cast<void>(d); // avoid warning for unused variable
	}

	void makePostExe()
	{
		uint8_t d[] = {(uint8_t)0, (ModuleCollection::makePostExe(), (uint8_t)0)...};
		static_cast<void>(d); // avoid warning for unused variable
	}
	 
	void makePreDel()
	{
		uint8_t d[] = {
			(uint8_t)0, (ModuleCollection::makePreDel(), (uint8_t)0)...
		};
		static_cast<void>(d); // avoid warning for unused variable
	}
		
};

