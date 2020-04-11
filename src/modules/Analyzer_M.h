#pragma once



template<uint16_t max_stack_usage, uint8_t data_filter_coefficient = 3> 
struct Analizer_M
{

	static_assert(max_stack_usage >= sizeof(uint32_t), "max_stack_usage must >= 4");
	
	using cycle_t = uint16_t;

	enum eAnalizeType{
		eEnd,
		eStackUsage,
		eExecTime,
		eCPUMeasure,
		eTypeCount
	};

	enum eResult{
		eNone,
		ePassed,
		eErrorMemoryLeak,
		eUserTerminated
	};

	void start(cycle_t inExeCount)
	{
		if(!isAnalizerAvailable()){
			return;
		}
		sExeCount = inExeCount;
		sCurAnalyzer = this;
		sType = eStackUsage;
		sResult = eNone;
		sTotalTime = getTime();
	}

	void stop()
	{
		terminate(eUserTerminated);
	}

	bool isAnalizerAvailable()
	{
		return (sCurAnalyzer != nullptr);
	}

	bool isAnalizerRunning()
	{
		return (sCurAnalyzer == this);
	}

	void setTimeBase(tick_t (*inGetTick)())
	{
		sGetTick = inGetTick;
	}
	
	template<typename T>
	void init(T *t) {}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) { return !isAnalizerRunning(); } 
	template<typename T>
	void makePreExe(T *t)
	{
		if(!isAnalizerRunning())
		{
			return;
		}

		switch(sType){
			case eStackUsage:
			{
				const auto size=max_stack_usage/sizeof(uint32_t);
				volatile uint32_t s[size];
				sSp = s;
				uint16_t i=0;
				while(i < size){
					s[i] = kStackPattern;
				}
			}
				break;
			case eExecTime:
				 sCurrExeTime = getTime();
				break;
			case eCPUMeasure:
				break;
		}
		
	}
	template<typename T>
	void makePostExe(T *t)
	{
		if(!isAnalizerRunning())
		{
			return;
		}

		switch(sType){
			case eStackUsage:
			{
				uint32_t dummy;
				if(sSp != &dummy){
					//stack overflow or memory leakage	
					terminate(eErrorMemoryLeak);
				}

				const auto size=max_stack_usage/sizeof(uint32_t);
				uint32_t s[size];
				uint16_t i=0;
				uint16_t currStackUsage = 0;
				while(i < size){
					if(s[i] == kStackPattern){
						// stack usage
						currStackUsage = i*sizeof(uint32_t);
						break;
					}
					i++;
				}
			
				sAverageStackUsage = smooth(sAverageStackUsage, currStackUsage);
				if( currStackUsage > sMaxStackUsage){
				 	sMaxStackUsage =  currStackUsage;
				}
				}
				break;
			case eExecTime:
				sCurrExeTime = getTime() - sCurrExeTime;
				sAverageExeTime  = smooth(sAverageExeTime, sCurrExeTime);
				if(sCurrExeTime > sMaxExeTime){
					sMaxExeTime =  sCurrExeTime;
				}
				break;
			case eCPUMeasure:
				break;
		}

		if(sExeCounter++ == sExeCount)
		{
			sType = (sType+1)%eTypeCount;
			sExeCounter = 0;
			if(sType == eEnd)
			{	// terminate analize
				terminate(ePassed);
			}			
		}
	}

	template<typename T>
	void makePreDel(T *t){}

	// stack
	static uint16_t sAverageStackUsage;
	static uint16_t sMaxStackUsage;

	// exe time
	static tick_t sAverageExeTime;
	static tick_t sMaxExeTime;

	// global
	static tick_t sTotalTime;

	static eResult sResult;
	
private:

	void terminate(eResult inResult)
	{
		sCurAnalyzer = nullptr;
		sTotalTime = getTime() - sTotalTime;
	}
	
	template<typename T>
	T smooth(T inPrev, T inNew)
	{
		(inPrev*(1<<(data_filter_coefficient-1)) + inNew)/(1<<data_filter_coefficient);
	}

	tick_t getTime()
	{
		return sGetTick();
	}
	
	static Analizer_M *sCurAnalyzer;

	static cycle_t sExeCounter;
	static cycle_t sExeCount;

	static eAnalizeType sType;

	// stack usage
	static uint32_t *sSp;
	

	// exe time
	static tick_t sCurrExeTime;
	static tick_t (*sGetTick)();
	
	static const uint32_t kStackPattern = 0xAACCBBDD;
};

