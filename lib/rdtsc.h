#pragma once

#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#define always_inline __always_inline
#else
#include <x86intrin.h>
#define always_inline __attribute__((__always_inline__))
#endif

always_inline uint64_t rdtsc() {
  return __rdtsc();
}

always_inline uint64_t rdtscp() {
  unsigned int aux;
  return __rdtscp(&aux);
}
