#pragma once



// contains an element of the specified type
template<typename content_t> 
struct Content_M
{	
	
	content_t& getContent()
	{
		return content;
	}
	
protected:

	template<typename T>
	void init(T *t) {}
	template<typename T>
	bool isExeReady(T *t) const { return true; }
	template<typename T>
	bool isDelReady(T *t) const { return true; } 
	template<typename T>
	void makePreExe(T *t){}
	template<typename T>
	void makePreDel(T *t){}
	template<typename T>
	void makePostExe(T *t){}

	content_t content;
};
