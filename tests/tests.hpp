#pragma once

#include <stdint.h>
#include "doctest.h"

uint32_t getMicros();
uint32_t getMillis();
void waitFor_ms(uint32_t inWait_ms);
