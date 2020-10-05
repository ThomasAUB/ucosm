#pragma once

// provides a module container

template<class...ModuleCollection> 
class ModuleMix_M : public ModuleCollection...
{

	using modules_t = std::tuple<ModuleCollection...>;

	template<size_t I>
	using getTypeAt = typename std::tuple_element<I, modules_t>::type;

	static constexpr size_t kModuleCount = std::tuple_size<modules_t>::value;
	
public:
					
	void init(){
		uint8_t d[] = {(uint8_t)0, (ModuleCollection::init(), (uint8_t)0)...};
		static_cast<void>(d); // avoids warning dor unused variables
	}
	
	template<size_t I=0>
	typename std::enable_if<I < kModuleCount, bool>::type 
	isExeReady(){
		if(static_cast<getTypeAt<I>*>(this)->isExeReady()){
			return isExeReady<I+1>();
		}
		return false;
	}
	
	template<size_t I=0>
	typename std::enable_if<I < kModuleCount, bool>::type 
	isDelReady(){
		if(static_cast<getTypeAt<I>*>(this)->isDelReady()){
			return isDelReady<I+1>();
		}
		return false;
	}

	void makePreDel(){
		uint8_t d[] = {(uint8_t)0, (ModuleCollection::makePreDel(), (uint8_t)0)...};
		static_cast<void>(d); // avoids warning dor unused variables
	}
	
	void makePreExe(){
		uint8_t d[] = {(uint8_t)0, (ModuleCollection::makePreExe(), (uint8_t)0)...};
		static_cast<void>(d); // avoids warning dor unused variables
	}
			
	void makePostExe(){
		uint8_t d[] = {(uint8_t)0, (ModuleCollection::makePostExe(), (uint8_t)0)...};
		static_cast<void>(d); // avoids warning dor unused variables
	}

private:

	
	template<size_t I=0>
	typename std::enable_if<I == kModuleCount, bool>::type 
	isExeReady(){ return true;	}

	template<size_t I=0>
	typename std::enable_if<I == kModuleCount, bool>::type 
	isDelReady(){ return true; }

};

