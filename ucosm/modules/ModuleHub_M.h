#pragma once


#include <tuple>

// provides a module container




template<class...ModuleCollection> 
class ModuleHub_M
{

	using modules_t = std::tuple<ModuleCollection...>;

	static constexpr size_t kModuleCount = std::tuple_size<modules_t>::value;

	modules_t mModules;
	
public:
	
	template<typename module_t>
	auto getModule(){
		return &std::get<module_t>(mModules);
	}

	template<size_t I>
	auto getModule(){
		return &std::get<I>(mModules);
	}
		
	//////////////////// init
		
	template<size_t I=0>
	typename std::enable_if<I < kModuleCount, void>::type 
	init(){
		std::get<I>(mModules).init();
		init<I+1>();
	}
	
	//////////////////// exe readdy
		
	template<size_t I=0>
	typename std::enable_if<I < kModuleCount, bool>::type 
	isExeReady(){
		if(std::get<I>(mModules).isExeReady()){
			return isExeReady<I+1>();
		}
		return false;
	}
	
	//////////////////// del ready
		
	template<size_t I=0>
	typename std::enable_if<I < kModuleCount, bool>::type 
	isDelReady(){
		if(std::get<I>(mModules).isDelReady()){
			return isDelReady<I+1>();
		}
		return false;
	}
	
	//////////////////// pre del
		
	template<size_t I=0>
	typename std::enable_if<I < kModuleCount, void>::type 
	makePreDel(){
		std::get<I>(mModules).makePreDel();
		makePreDel<I+1>();
	}
	
	//////////////////// pre exe
		
	template<size_t I=0>
	typename std::enable_if<I < kModuleCount, void>::type 
	makePreExe(){
		std::get<I>(mModules).makePreExe();
		makePreExe<I+1>();
	}
	
	//////////////////// post exe
		
	template<size_t I=0>
	typename std::enable_if<I < kModuleCount, void>::type 
	makePostExe(){
		std::get<I>(mModules).makePostExe();
		makePostExe<I+1>();
	}
	
	////////////////////
	
private:

	template<size_t I=0>
	typename std::enable_if<I == kModuleCount, void>::type 
	init(){}

	template<size_t I=0>
	typename std::enable_if<I == kModuleCount, bool>::type 
	isExeReady(){ return true;	}

	template<size_t I=0>
	typename std::enable_if<I == kModuleCount, bool>::type 
	isDelReady(){ return true; }

	template<size_t I=0>
	typename std::enable_if<I == kModuleCount, void>::type 
	makePreDel(){}

	template<size_t I=0>
	typename std::enable_if<I == kModuleCount, void>::type 
	makePreExe(){}

	template<size_t I=0>
	typename std::enable_if<I == kModuleCount, void>::type 
	makePostExe(){}
	
};

