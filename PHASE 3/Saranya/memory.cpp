#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <iomanip>

using namespace std;

#define CORE_COUNT 1
#define MEMORY_SIZE 4096
#define REGISTER_COUNT 32

#define L1_CACHE_SIZE 32
#define L2_CACHE_SIZE 1024
#define LINE_SIZE 16
#define ASSOCIATIVITY 1

int cache_l1_hits=0;
int cache_l1_misses=0;
int cache_l2_hits=0;
int cache_l2_misses=0;
int memory_accesses=0;
uint8_t memory_main[MEMORY_SIZE] = {0};
int registers[REGISTER_COUNT] = {0};

uint32_t heap_pointers[4] = {0, 1024, 2048, 3072};
int core_id = 0;

int getSetsL1() { return L1_CACHE_SIZE / LINE_SIZE; }
int getSetsL2() { return L2_CACHE_SIZE / LINE_SIZE; }
int log2int(int x) { return static_cast<int>(log2(x)); }

struct CacheLine {
    bool valid = false;
    uint32_t tag = 0;
    uint8_t data[LINE_SIZE] = {0}; 
};

int L1_SETS = getSetsL1();
int L2_SETS = getSetsL2();

CacheLine l1_cache[CORE_COUNT][L1_CACHE_SIZE]; 
CacheLine l2_cache[L2_CACHE_SIZE];           

uint32_t getL1Index(uint32_t address) {
    return (address >> log2int(LINE_SIZE)) & (L1_SETS - 1);
}
uint32_t getL2Index(uint32_t address) {
    return (address >> log2int(LINE_SIZE)) & (L2_SETS - 1);
}
uint32_t getTag_L1(uint32_t address) {
    cout<<"Tag L1 address: " << address << endl;
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
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }
    return heap_pointers[core_id];
}

uint32_t allocate_memory(size_t num_elements, int core_id) {
    uint32_t base_address = allocate_base_adress(core_id);
    uint32_t &heap_pointer = heap_pointers[core_id];

    if (heap_pointer + (num_elements * 4) >= base_address + 1024) {
        std::cerr << "Error: Core " << core_id << " exceeded its 1KB memory limit!\n";
        exit(1);
    }

    uint32_t allocated_address = heap_pointer;
    heap_pointer += num_elements * 4;
    return allocated_address;
}

int lw(int address, int core_id) {
    memory_accesses++;
    if (address < 0 || address + 3 >= MEMORY_SIZE) {
        std::cerr << "Memory read out of bounds!\n";
        exit(1);
    }

    if (address % 4 != 0) {
        std::cerr << "Unaligned memory access!\n";
        exit(1);
    }

    uint32_t offset = address % LINE_SIZE;
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getTag_L1(address);
    CacheLine &l1_line = l1_cache[core_id][l1_idx];

    if (l1_line.valid && l1_line.tag == l1_tag) {
        cache_l1_hits++;
        int word;
        std::memcpy(&word, &l1_line.data[offset], sizeof(int));
        return word;
    }
    else {
        cache_l1_misses++;
    }
    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    CacheLine &l2_line = l2_cache[l2_idx];

    if (l2_line.valid && l2_line.tag == l2_tag) {
        cache_l2_hits++;
        l1_line = l2_line;
        int word;
        std::memcpy(&word, &l1_line.data[offset], sizeof(int));
        return word;
    }
    else{
        cache_l2_misses++;
    }

    uint32_t block_start = address - offset;
    int word;
    std::memcpy(&word, &memory_main[address], sizeof(int));
    // Load 16 bytes (1 line) from memory into L2
    std::memcpy(l2_line.data, &memory_main[block_start], LINE_SIZE);
    l2_line.tag = l2_tag;
    l2_line.valid = true;

    // Copy into L1
    l1_line = l2_line;
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
    uint32_t l1_tag = getTag_L1(address);
    uint32_t l1_idx = getL1Index(address);
    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    CacheLine &l1_line = l1_cache[core_id][l1_idx];
    CacheLine &l2_line = l2_cache[l2_idx];

    uint32_t block_start = address - offset;
    std::memcpy(l1_line.data, &memory_main[block_start], LINE_SIZE);
    std::memcpy(l2_line.data, &memory_main[block_start], LINE_SIZE);
    l1_line.valid = l2_line.valid = true;
    l1_line.tag = l2_line.tag = l2_tag;
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

void printCaches() {
    std::cout << "\n=== L1 Cache State (Matrix View Per Core) ===\n";
    for (int cid = 0; cid < CORE_COUNT; ++cid) {
        std::cout << "\nCore " << cid << " L1 Cache:\n";
        // Print tag row
        std::cout << "Tag:      ";
        for (int i = 0; i < L1_SETS; ++i)
            std::cout << " " << std::setw(6)<< l1_cache[cid][i].tag;
        std::cout << "\n";

        // Print valid bit row
        std::cout << "Valid:    ";
        for (int i = 0; i < L1_SETS; ++i)
            std::cout << " " << std::setw(6) << l1_cache[cid][i].valid;
        std::cout << "\n";

        for (int b = 0; b < LINE_SIZE; b += 4) {
            std::cout << "Data["<< b/4 << "]:  ";
            for (int i = 0; i < L1_SETS; ++i) {
                int word;
                std::memcpy(&word, &l1_cache[cid][i].data[b], sizeof(int));
                std::cout << " " << std::setw(6) << word;
            }
            std::cout << "\n";
        }
        
    }

    std::cout << "\n=== Shared L2 Cache State (Matrix View) ===\n";

    // Print tag row
    std::cout << "Tag:      ";
    for (int i = 0; i < L2_SETS; ++i)
        std::cout << " " << l2_cache[i].tag;
    std::cout << "\n";

    // Print valid bit row
    std::cout << "Valid:    ";
    for (int i = 0; i < L2_SETS; ++i)
        std::cout << " " << l2_cache[i].valid;
    std::cout << "\n";

    
    for (int b = 0; b < LINE_SIZE; b += 4) {
        std::cout << "Data[" << std::setw(2) << b/4 << "]:  ";
        for (int i = 0; i < L2_SETS; ++i) {
            int word;
            std::memcpy(&word, &l2_cache[i].data[b], sizeof(int));
            std::cout << " " << word;
        }
        std::cout << "\n";
    }
}

