#pragma once
#include <cstdint>
#include <vector>
#include "../common/constants.hpp"

struct CacheLine {
    bool valid = false;
    uint32_t tag = 0;
    std::vector<uint8_t> line_data;
    int counter = 0;
};

extern std::vector<CacheLine>** l1_cache;
extern std::vector<CacheLine>* l2_cache;

extern int cache_l1_hits[CORE_COUNT];
extern int cache_l1_misses[CORE_COUNT];
extern int cache_l2_hits;
extern int cache_l2_misses;
extern int memory_accesses;

void initializeDataCaches();
int getSetsL1();
int getSetsL2();

int lw(int address); // legacy single-argument debug helper (no cache lookup)
int lw(int address, int core_id);
void sw(int address, int value, int core_id);

void printCaches();
