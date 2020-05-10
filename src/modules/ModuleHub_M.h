#pragma once


#include <tuple>

// provides a module container







template<class ...ModuleCollection> 
struct ModuleHub1_M : public ModuleCollection...
{

	ModuleHub1_M()
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





template<class ...ModuleCollection> 
class ModuleHub2_M
{

	using items_t = std::tuple<ModuleCollection...>;
	items_t mItemModules;

public:

	template<typename T>
	T* get() {
		return &std::get<T>(mItemModules);
	}


	template<typename T>
	bool contains(T *t){
		return (get<T>() == t);
	}

	template<size_t I = 0>
	void init() {
		std::get<I>(mItemModules).init();
		if constexpr (I+1 != std::tuple_size<items_t>::value)
		    init<I+1>();
	}

	template<size_t I = 0>
	bool isExeReady() {
		if constexpr (I+1 != std::tuple_size<items_t>::value){
			if(std::get<I>(mItemModules).isExeReady()){
				return isExeReady<I+1>();
			}else{
				return false;
			}
		}else{
			return std::get<I>(mItemModules).isExeReady();
		}
	}
	
	template<size_t I = 0>
	bool isDelReady() {
		if constexpr (I+1 != std::tuple_size<items_t>::value){
			if(std::get<I>(mItemModules).isDelReady()){
				return isDelReady<I+1>();
			}else{
				return false;
			}
		}else{ 
			return std::get<I>(mItemModules).isDelReady();
		}
	}

	template<size_t I = 0>
	void makePreExe() {
		std::get<I>(mItemModules).makePreExe();
		if constexpr (I+1 != std::tuple_size<items_t>::value)
		    makePreExe<I+1>();
	}

	template<size_t I = 0>
	void makePostExe() {
		std::get<I>(mItemModules).makePostExe();
		if constexpr (I+1 != std::tuple_size<items_t>::value)
		    makePostExe<I+1>();
	}
	
	template<size_t I = 0>
	void makePreDel() {
		std::get<I>(mItemModules).makePreDel();
		if constexpr (I+1 != std::tuple_size<items_t>::value)
		    makePreDel<I+1>();
	}
};