#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <iomanip>

using namespace std;

#define CORE_COUNT 4
#define MEMORY_SIZE 4096
#define REGISTER_COUNT 32
#define L1_CACHE_SIZE 128
#define L2_CACHE_SIZE 1024
#define LINE_SIZE 16
#define ASSOCIATIVITY_L1 2
#define ASSOCIATIVITY_L2 4

int cache_l1_hits = 0;
int cache_l1_misses = 0;
int cache_l2_hits = 0;
int cache_l2_misses = 0;
int memory_accesses = 0;
uint8_t memory_main[MEMORY_SIZE] = {0};
int registers[REGISTER_COUNT] = {0};
uint32_t heap_pointers[4] = {0, 1024, 2048, 3072};

int log2int(int x) { return static_cast<int>(log2(x)); }
int getSetsL1() { return L1_CACHE_SIZE / (LINE_SIZE * ASSOCIATIVITY_L1); }
int getSetsL2() { return L2_CACHE_SIZE / (LINE_SIZE * ASSOCIATIVITY_L2); }

constexpr int L1_SETS = L1_CACHE_SIZE / (LINE_SIZE * ASSOCIATIVITY_L1);
constexpr int L2_SETS = L2_CACHE_SIZE / (LINE_SIZE * ASSOCIATIVITY_L2);

struct CacheLine {
    bool valid = false;
    uint32_t tag = 0;
    uint8_t data[LINE_SIZE] = {0};
    int counter = 0;
};

vector<CacheLine> l1_cache[CORE_COUNT][L1_SETS]; // max 64 sets
vector<CacheLine> l2_cache[L2_SETS];             // max 64 sets
void initializeCaches() {
    for (int i = 0; i < CORE_COUNT; i++) {
        for (int j = 0; j < L1_SETS; j++) {
            l1_cache[i][j].resize(ASSOCIATIVITY_L1);
        }
    }
    for (int i = 0; i < L2_SETS; i++) {
        l2_cache[i].resize(ASSOCIATIVITY_L2);
    }
}

uint32_t getL1Index(uint32_t address) {
    return (address >> log2int(LINE_SIZE)) & (L1_SETS - 1);
}

uint32_t getL2Index(uint32_t address) {
    return (address >> log2int(LINE_SIZE)) & (L2_SETS - 1);
}

uint32_t getTag_L1(uint32_t address) {
    return address >> (log2int(LINE_SIZE) + log2int(L1_SETS));
}

uint32_t getTag_L2(uint32_t address) {
    return address >> (log2int(LINE_SIZE) + log2int(L2_SETS));
}

uint32_t getOffset(uint32_t address) {
    return address & (LINE_SIZE - 1);
}

uint32_t allocate_base_adress(int core_id) {
    if (core_id < 0 || core_id >= 4) {
        cerr << "Invalid core ID: " << core_id << endl;
        exit(1);
    }
    return heap_pointers[core_id];
}

uint32_t allocate_memory(size_t num_elements, int core_id) {
    uint32_t base_address = allocate_base_adress(core_id);
    uint32_t &heap_pointer = heap_pointers[core_id];

    if (heap_pointer + (num_elements * 4) >= base_address + 1024) {
        cerr << "Error: Core " << core_id << " exceeded its 1KB memory limit!\n";
        exit(1);
    }

    uint32_t allocated_address = heap_pointer;
    heap_pointer += num_elements * 4;
    return allocated_address;
}

int getLRUWay(vector<CacheLine> &set) {
    int lru_way = 0;
    int max_counter = -1;
    for (int way = 0; way < set.size(); ++way) {
        if (!set[way].valid) return way;
        if (set[way].counter > max_counter) {
            max_counter = set[way].counter;
            lru_way = way;
        }
    }
    return lru_way;
}

void updateLRU(vector<CacheLine> &set, int accessed_way) {
    for (int way = 0; way < set.size(); ++way) {
        if (way == accessed_way) {
            set[way].counter = 0;
        } else if (set[way].valid) {
            set[way].counter++;
        }
    }
}

int lw(int address, int core_id) {
    memory_accesses++;

    if (address < 0 || address + 3 >= MEMORY_SIZE || address % 4 != 0) {
        cerr << "Invalid memory read!\n";
        exit(1);
    }

    uint32_t offset = getOffset(address);
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getTag_L1(address);

    auto &l1_set = l1_cache[core_id][l1_idx];
    if (l1_set.empty()) l1_set.resize(ASSOCIATIVITY_L1);

    for (int way = 0; way < ASSOCIATIVITY_L1; ++way) {
        if (l1_set[way].valid && l1_set[way].tag == l1_tag) {
            cache_l1_hits++;
            updateLRU(l1_set, way);
            int word;
            memcpy(&word, &l1_set[way].data[offset], sizeof(int));
            return word;
        }
    }
    cache_l1_misses++;

    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    auto &l2_set = l2_cache[l2_idx];
    if (l2_set.empty()) l2_set.resize(ASSOCIATIVITY_L2);

    for (int way = 0; way < ASSOCIATIVITY_L2; ++way) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            cache_l2_hits++;
            updateLRU(l2_set, way);
            int l1_replace = getLRUWay(l1_set);
            l1_set[l1_replace] = l2_set[way];
            updateLRU(l1_set, l1_replace);
            int word;
            memcpy(&word, &l1_set[l1_replace].data[offset], sizeof(int));
            return word;
        }
    }
    cache_l2_misses++;

    int l2_replace = 0;
    uint32_t block_start = address - offset;
    memcpy(l2_set[l2_replace].data, &memory_main[block_start], LINE_SIZE);
    l2_set[l2_replace].tag = l2_tag;
    l2_set[l2_replace].valid = true;
    updateLRU(l2_set, l2_replace);

    int l1_replace = 0;
    l1_set[l1_replace] = l2_set[l2_replace];
    updateLRU(l1_set, l1_replace);

    int word;
    memcpy(&word, &l1_set[l1_replace].data[offset], sizeof(int));
    return word;
}



void sw(int address, int value, int core_id) {
    memory_accesses++;
    if (address < 0 || address + 3 >= MEMORY_SIZE) {
        std::cerr << "Memory write out of bounds!\n";
        exit(1);
    }


    uint32_t offset = getOffset(address);
    if (offset % 4 != 0) {
        std::cerr << "Unaligned memory write!\n";
        exit(1);
    }
    cout<<"Storing value: " << value << " at address: " << address << endl;
    std::memcpy(&memory_main[address], &value, sizeof(int));
    /*uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getTag_L1(address);
    CacheLine &l1_line = l1_cache[core_id][l1_idx];
    uint32_t block_start = address - offset;
    if (l1_line.valid && l1_line.tag == l1_tag ) {
        cache_l1_hits++;
        int word;
        std::memcpy(&l1_line.data[offset], &value, sizeof(int));
       
    }
    else {
        cache_l1_misses++;
        std::memcpy(l1_line.data, &memory_main[block_start], LINE_SIZE);
        l1_line.tag = l1_tag;
        l1_line.valid = true;
    }
    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    CacheLine &l2_line = l2_cache[l2_idx];


    if (l2_line.valid && l2_line.tag == l2_tag) {
        cache_l2_hits++;
        l1_line = l2_line;
        int word;
        std::memcpy(&l2_line.data[offset], &value, sizeof(int));
        l2_line.valid = true;
       
    }
    else{
        cache_l2_misses++;
        std::memcpy(l2_line.data, &memory_main[block_start], LINE_SIZE);
        l2_line.tag = l2_tag;
    }*/




}


void sw1(int address, int value, int core_id) {
    int base_address = (core_id) * 1024;
    int max_address = base_address + 1024;
    if (address < 0 || address >= MEMORY_SIZE) {
        std::cerr << "Error: Core " << core_id << " tried to access memory out of its range!\n";
        exit(1);
    }
   
    std::memcpy(&memory_main[address], &value, sizeof(int));
    cout << "Stored value: " << value << " at address: " << address << endl;


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
    cout << "L1 Cache Hits: " << cache_l1_hits << endl;
    cout << "L1 Cache Misses: " << cache_l1_misses << endl;
    cout << "L2 Cache Hits: " << cache_l2_hits << endl;
    cout << "L2 Cache Misses: " << cache_l2_misses << endl;
    cout << "Memory Accesses: " << memory_accesses << endl;
    cout<< "Hit Rate L1: " << (float)cache_l1_hits/(memory_accesses) * 100 << "%" << endl;
    cout<< "Hit Rate L2: " << (float)cache_l2_hits/(memory_accesses) * 100 << "%" << endl;
    cout<< "Miss Rate L1: " << (float)cache_l1_misses/(memory_accesses) * 100 << "%" << endl;
    cout<< "Miss Rate L2: " << (float)cache_l2_misses/(memory_accesses) * 100 << "%" << endl;
}

