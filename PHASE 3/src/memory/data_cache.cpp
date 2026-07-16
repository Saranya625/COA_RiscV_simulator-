#include "data_cache.hpp"
#include "memory_config.hpp"
#include "cache_replacement.hpp"
#include "main_memory.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

using namespace std;

vector<CacheLine>** l1_cache = nullptr;
vector<CacheLine>* l2_cache = nullptr;

int cache_l1_hits[CORE_COUNT] = {0};
int cache_l1_misses[CORE_COUNT] = {0};
int cache_l2_hits = 0;
int cache_l2_misses = 0;
int memory_accesses = 0;

void initializeDataCaches() {
    L1_SETS = l1_cache_size / (line_size * associativity_l1);
    L2_SETS = l2_cache_size / (line_size * associativity_l2);

    l1_cache = new vector<CacheLine>*[CORE_COUNT];
    for (int i = 0; i < CORE_COUNT; i++) {
        l1_cache[i] = new vector<CacheLine>[L1_SETS];
        for (int j = 0; j < L1_SETS; j++) {
            l1_cache[i][j].resize(associativity_l1);
             for (int k = 0; k < associativity_l1; k++) {
                l1_cache[i][j][k].line_data.resize(line_size,0);   
            }
            
        }
    }

    l2_cache = new vector<CacheLine>[L2_SETS];
    for (int i = 0; i < L2_SETS; i++) {
        l2_cache[i].resize(associativity_l2);
        for (int j = 0; j < associativity_l2; j++) {
            l2_cache[i][j].line_data.resize(line_size,0);
        }
    }
}

int getSetsL1() { return l1_cache_size /( line_size * associativity_l1); }
int getSetsL2() { return l2_cache_size / (line_size * associativity_l2); }

int lw(int address) {
    if (address < 0 || address > MEMORY_SIZE - sizeof(int)) {
        throw std::out_of_range("Memory access out of bounds");
    }
    int value;
    std::memcpy(&value, &memory_main[address], sizeof(int));
    
    
    return value;
}

int lw(int address, int core_id) {

    if (address < 0 || address + 3 >= MEMORY_SIZE|| address % 4 != 0) {
        cerr << "Invalid memory read!\n";
        exit(1);
    }
    uint32_t offset = getOffset(address);
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getTag_L1(address);
    auto& l1_set = l1_cache[core_id][l1_idx];
    if (l1_set.empty()) l1_set.resize(associativity_l1);
    for (int way = 0; way < associativity_l1; ++way) {
        if (l1_set[way].valid && l1_set[way].tag == l1_tag) {
            cache_l1_hits[core_id]++;
            updateReplacement(l1_set, way);
            memory_latency=l1_latency;
            int word;
            memcpy(&word, &l1_set[way].line_data[offset], sizeof(int));
            return word;
        }
    }
    cache_l1_misses[core_id]++;

    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    auto& l2_set = l2_cache[l2_idx];
    if (l2_set.empty()) l2_set.resize(associativity_l2);

    for (int way = 0; way < associativity_l2; ++way) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            cache_l2_hits++;
            memory_latency=l2_latency;
            updateReplacement(l2_set, way);
            int l1_replace = get_replacement_way(l1_set);
            for(int i = 0; i < line_size; i++) {
                l1_set[l1_replace].line_data[i] = l2_set[way].line_data[i];
            }
            updateReplacement(l1_set, l1_replace);
            int word;
            memcpy(&word, &l1_set[l1_replace].line_data[offset], sizeof(int));
            return word;
        }
    }

    cache_l2_misses++;
    uint32_t block_start = address - offset;
    int l1 = get_replacement_way(l1_set);   
    for (int i = 0; i < line_size; i++) {
        l1_set[l1].line_data[i] = memory_main[block_start + i];
    }
    l1_set[l1].tag = l1_tag;
    l1_set[l1].valid = true;
    updateReplacement(l1_set, l1);
    int l2_replace = get_replacement_way(l2_set);
    for (int i = 0; i < line_size; i++) {
        l2_set[l2_replace].line_data[i] = memory_main[block_start + i];
    }
    l2_set[l2_replace].tag = l2_tag;
    l2_set[l2_replace].valid = true;
    updateReplacement(l2_set, l2_replace);

    memory_latency= main_memory_latency;
    int word;
    memcpy(&word, &l1_set[l1].line_data[offset], sizeof(int));
    return word;
}

void sw(int address, int value, int core_id) {
    if (address < 0 || address + 3 >= MEMORY_SIZE || address % 4 != 0) {
        std::cerr << "Error: Core " << core_id << " tried to write to invalid memory address!\n";
        exit(1);
    }

    // Write-through: main memory is always updated immediately, regardless
    // of whether the write hits in a cache.
    std::memcpy(&memory_main[address], &value, sizeof(int));

    uint32_t offset = getOffset(address);
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getTag_L1(address);
    auto& l1_set = l1_cache[core_id][l1_idx];
    if (l1_set.empty()) l1_set.resize(associativity_l1);

    for (int way = 0; way < associativity_l1; ++way) {
        if (l1_set[way].valid && l1_set[way].tag == l1_tag) {
            std::memcpy(&l1_set[way].line_data[offset], &value, sizeof(int));
            updateReplacement(l1_set, way);
            cache_l1_hits[core_id]++;
            memory_latency = l1_latency;
            return;
        }
    }
    cache_l1_misses[core_id]++;

    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    auto& l2_set = l2_cache[l2_idx];
    if (l2_set.empty()) l2_set.resize(associativity_l2);
    uint32_t block_start = address - offset;

    for (int way = 0; way < associativity_l2; ++way) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            std::memcpy(&l2_set[way].line_data[offset], &value, sizeof(int));
            updateReplacement(l2_set, way);
            cache_l2_hits++;
            memory_latency = l2_latency;

            // Bring the now up-to-date L2 line into L1 too.
            int l1_replace = get_replacement_way(l1_set);
            for (int i = 0; i < line_size; i++) {
                l1_set[l1_replace].line_data[i] = memory_main[block_start + i];
            }
            l1_set[l1_replace].tag = l1_tag;
            l1_set[l1_replace].valid = true;
            updateReplacement(l1_set, l1_replace);
            return;
        }
    }

    // Miss in both L1 and L2: fetch the block from main memory into both.
    cache_l2_misses++;
    memory_latency = main_memory_latency;

    int l1 = get_replacement_way(l1_set);
    for (int i = 0; i < line_size; i++) {
        l1_set[l1].line_data[i] = memory_main[block_start + i];
    }
    l1_set[l1].tag = l1_tag;
    l1_set[l1].valid = true;
    updateReplacement(l1_set, l1);

    int l2_replace = get_replacement_way(l2_set);
    for (int i = 0; i < line_size; i++) {
        l2_set[l2_replace].line_data[i] = memory_main[block_start + i];
    }
    l2_set[l2_replace].tag = l2_tag;
    l2_set[l2_replace].valid = true;
    updateReplacement(l2_set, l2_replace);
}

void printCaches() {
    cout << "        L1 Cache         \n";
    cout << "=========================\n";

    for (int core = 0; core < CORE_COUNT; ++core) {
        cout << "Core " << core << ":\n";
        for (int set = 0; set < L1_SETS; ++set) {
            cout << "  Set " << set << ":\n";
            for (size_t way = 0; way < l1_cache[core][set].size(); ++way) {
                const CacheLine& line = l1_cache[core][set][way];
                cout << " Way " << way << "|Valid: " << line.valid
                     << " |Tag: 0x" << hex << line.tag << dec
                     << " |Counter: " << line.counter
                     << " |Data: ";
                for (size_t i = 0; i < (size_t)line_size; i += 4) {
             cout << dec << static_cast<int>(line.line_data[i]) << " ";
            }  
                cout << dec << "\n";
            }
        }
        cout << "\n";
    }

    cout << "=========================\n";
    cout << "        L2 Cache         \n";
    cout << "=========================\n";

    for (int set = 0; set < L2_SETS; ++set) {
        cout << "Set " << set << ":\n";
        for (size_t way = 0; way < l2_cache[set].size(); ++way) {
            const CacheLine& line = l2_cache[set][way];
            cout << "  Way " << way << " | Valid: " << line.valid
                 << " | Tag: 0x" << hex << line.tag << dec
                 << " | Counter: " << line.counter
                 << " | Data: ";
             for (size_t i = 0; i < (size_t)line_size; i += 4) {
             cout << dec << static_cast<int>(line.line_data[i]) << " ";
         }

            cout << dec << "\n";
        }
        cout << "\n";
    }
}
