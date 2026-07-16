#include "memory_system.hpp"
#include "memory_config.hpp"
#include "main_memory.hpp"
#include "data_cache.hpp"
#include "instruction_cache.hpp"
#include "../common/constants.hpp"
#include <iostream>
#include <cstring>

using namespace std;

void initializeSystem(int line_size1 , int l1_size, int l2_size, int l1_assoc, int l2_assoc, int l1_lat, int l2_lat, string repl_policy, int main_mem_lat) {
    // Input config gives line size in words; internal cache logic uses bytes.
    line_size = line_size1 * 4;
    l1_cache_size = l1_size;
    l2_cache_size = l2_size;
    associativity_l1 = l1_assoc;
    associativity_l2 = l2_assoc;
    l1_latency = l1_lat;
    l2_latency = l2_lat;
    replacement_policy = repl_policy;
    main_memory_latency = main_mem_lat;
    
    initializeDataCaches();
    init_instruction_caches();
}

void printMemory() {
    std::cout << "\n=== Memory State ===\n";
    for (int addr = 0; addr < MEMORY_SIZE; addr += 4) {
        int value;
        std::memcpy(&value, &memory_main[addr], sizeof(int));
        std::cout << "0x" << std::hex << addr << ": " << std::dec << value << "\t";
        if ((addr / 4 + 1) % 8 == 0) std::cout << std::endl;
    }
    cout << "\n=== Cache Statistics ===\n";
    for(int i=0; i<CORE_COUNT; i++){
        cout << "Core " << i << " L1 Cache Hits: " << cache_l1_hits[i] << endl;
    }
    for(int i=0; i<CORE_COUNT; i++){
        cout << "Core " << i << " L1 Cache Misses: " << cache_l1_misses[i] << endl;
    }
    cout << "L2 Cache Hits: " << cache_l2_hits << endl;
    cout << "L2 Cache Misses: " << cache_l2_misses << endl;
    cout << "Memory Accesses: " << memory_accesses << endl;
    for(int i=0; i<CORE_COUNT; i++){
        int l1_total = cache_l1_hits[i] + cache_l1_misses[i];
        float hit_l1 = (l1_total == 0) ? 0.0f : (float)cache_l1_hits[i] / l1_total * 100.0f;
        cout<< "Hit Rate L1: " << hit_l1 << "%" << endl;
    }
    int l2_total = cache_l2_hits + cache_l2_misses;
    float hit_l2 = (l2_total == 0) ? 0.0f : (float)cache_l2_hits / l2_total * 100.0f;
    cout<< "Hit Rate L2: " << hit_l2 << "%" << endl;
   for(int i=0; i<CORE_COUNT; i++){
        int l1_total = cache_l1_hits[i] + cache_l1_misses[i];
        float miss_l1 = (l1_total == 0) ? 0.0f : (float)cache_l1_misses[i] / l1_total * 100.0f;
        cout<< "Miss Rate L1: " << miss_l1 << "%" << endl;
    }
    float miss_l2 = (l2_total == 0) ? 0.0f : (float)cache_l2_misses / l2_total * 100.0f;
    cout<< "Miss Rate L2: " << miss_l2 << "%" << endl;
}
