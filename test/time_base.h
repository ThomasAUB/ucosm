#ifndef TEST_UCOSM_TIMEBASE_H
#define TEST_UCOSM_TIMEBASE_H

#include <ctime>

#include "kernel.h"
#include "traits.h"

tick_t getTick(){
    return time(0)*1000;
}
tick_t (*SysKernelData::sGetTick)() = &getTick;

#endif