#pragma once
#include <cstdint>
#include <string>

// Shared cache/memory configuration, populated once by initializeSystem()
// and consumed by both the data cache and the instruction cache.

extern int line_size;          // cache line size, in bytes
extern int l1_cache_size;
extern int l2_cache_size;
extern int associativity_l1;
extern int associativity_l2;
extern int l1_latency;
extern int l2_latency;
extern std::string replacement_policy;
extern int main_memory_latency;
extern int memory_latency;     // latency resolved by the most recent lw()/sw()
extern int fetch_latency;      // latency resolved by the most recent fetch_instruction()
extern int L1_SETS;
extern int L2_SETS;

int log2int(int x);
uint32_t getL1Index(uint32_t address);
uint32_t getL2Index(uint32_t address);
uint32_t getTag_L1(uint32_t address);
uint32_t getTag_L2(uint32_t address);
uint32_t getOffset(uint32_t address);
