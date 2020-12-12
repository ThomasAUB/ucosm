#pragma once

#include <chrono>
uint32_t getTick() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}