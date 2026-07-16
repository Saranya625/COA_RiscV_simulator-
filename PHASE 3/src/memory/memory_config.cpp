#include "memory_config.hpp"
#include <cmath>

int line_size = 0;
int l1_cache_size = 0;
int l2_cache_size = 0;
int associativity_l1 = 0;
int associativity_l2 = 0;
int l1_latency = 0;
int l2_latency = 0;
std::string replacement_policy;
int main_memory_latency = 0;
int memory_latency = 0;
int fetch_latency = 0;
int L1_SETS = 0;
int L2_SETS = 0;

int log2int(int x) { return static_cast<int>(log2(x)); }

uint32_t getL1Index(uint32_t address) {
    return (address >> log2int(line_size)) & (L1_SETS - 1);
}

uint32_t getL2Index(uint32_t address) {
    return (address >> log2int(line_size)) & (L2_SETS - 1);
}

uint32_t getTag_L1(uint32_t address) {
    return address >> (log2int(line_size) + log2int(L1_SETS));
}

uint32_t getTag_L2(uint32_t address) {
    return address >> (log2int(line_size) + log2int(L2_SETS));
}

uint32_t getOffset(uint32_t address) {
    return address & (line_size - 1);
}
