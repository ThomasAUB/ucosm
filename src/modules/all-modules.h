#pragma once


//16 modules

// executers
#include "Interval_M.h"
#include "Priority_M.h"
#include "Conditional_M.h"

#include "Coroutine_M.h" // requires delay but in macro

// containers
#include "ModuleKit_M.h"
#include "Content_M.h"
#include "Buffer_M.h"
#include "Signal_M.h"	// requires status (but discutable and bypassable)
//#include "MemAllocator_M.h" not working yet

// linkers
#include "Parent_M.h"
#include "LinkedList_M.h" // requires status (cbypassable)

// 
#include "Status_M.h"
#include "StatusNotify_M.h"	// requires status (could be merged?)

// analyzers
#include "Stack_Usage_M.h"
#include "CPU_Usage_M.h"


#include "void_M.h"
