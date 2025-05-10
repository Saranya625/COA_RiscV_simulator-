#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <vector>
#include <fstream>
#include <string>
#include <unordered_map>
#include <iomanip>

using namespace std;

// Default values, will be overridden by specifications.txt
int CORE_COUNT = 4;
int MEMORY_SIZE = 4096;
int REGISTER_COUNT = 32;
int L1_CACHE_SIZE = 64;        // Changed from constant to variable
int L2_CACHE_SIZE = 1024;      // Changed from constant to variable
int LINE_SIZE = 16;
int ASSOCIATIVITY_L1 = 2;      // Changed from constant to variable
int ASSOCIATIVITY_L2 = 4;      // Changed from constant to variable
int L1_LATENCY = 1;            // Added latency variable
int L2_LATENCY = 3;            // Added latency variable
int MAIN_MEMORY_LATENCY = 7;   // Added latency variable

// Cache statistics
int cache_l1_hits = 0;
int cache_l1_misses = 0;
int cache_l2_hits = 0;
int cache_l2_misses = 0;
int memory_accesses = 0;
int stall_cycles = 0;          // Added to track stall cycles

// Memory and registers
uint8_t memory_main[4096] = {0};  // Using fixed size for simplicity
int registers[32] = {0};          // Using fixed size for simplicity
uint32_t heap_pointers[4] = {0, 1024, 2048, 3072};

// Derived cache parameters (will be recalculated after loading specs)
int L1_SETS;
int L2_SETS;

struct CacheLine {
    bool valid = false;
    uint32_t tag = 0;
    uint8_t data[64] = {0};  // Using max possible line size
    int counter = 0;         // For LRU implementation
};

// Dynamic cache structures
vector<vector<vector<CacheLine>>> l1_cache;  // [core][set][way]
vector<vector<CacheLine>> l2_cache;          // [set][way]

// Helper functions
int log2int(int x) { 
    return static_cast<int>(log2(x)); 
}

// Function to read specifications from file
void readSpecifications(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open specifications file: " << filename << endl;
        exit(1);
    }

    string line;
    while (getline(file, line)) {
        // Remove whitespace and check for empty lines
        line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (line.empty() || line[0] == '}' || line[0] == '[' || line[0] == ']') continue;

        // Remove trailing characters
        size_t colonPos = line.find(":");
        if (colonPos != string::npos) {
            string key = line.substr(0, colonPos);
            string value = line.substr(colonPos + 1);
            
            // Remove any trailing non-digit characters
            value.erase(find_if(value.rbegin(), value.rend(), ::isdigit).base(), value.end());
            
            if (key == "L1_cache_size") {
                L1_CACHE_SIZE = stoi(value);
            } else if (key == "L2_cache_size") {
                L2_CACHE_SIZE = stoi(value);
            } else if (key == "L1_associativity") {
                ASSOCIATIVITY_L1 = stoi(value);
            } else if (key == "L2_associativity") {
                ASSOCIATIVITY_L2 = stoi(value);
            } else if (key == "L1_latency") {
                L1_LATENCY = stoi(value);
            } else if (key == "L2_latency") {
                L2_LATENCY = stoi(value);
            } else if (key == "Main_memory") {
                MAIN_MEMORY_LATENCY = stoi(value);
            }
        }
    }
    
    file.close();
    
    // Calculate derived parameters
    L1_SETS = L1_CACHE_SIZE / (LINE_SIZE * ASSOCIATIVITY_L1);
    L2_SETS = L2_CACHE_SIZE / (LINE_SIZE * ASSOCIATIVITY_L2);
    
    // Print loaded specifications for verification
    cout << "=== Loaded Cache Specifications ===" << endl;
    cout << "L1 Cache Size: " << L1_CACHE_SIZE << " bytes" << endl;
    cout << "L2 Cache Size: " << L2_CACHE_SIZE << " bytes" << endl;
    cout << "L1 Associativity: " << ASSOCIATIVITY_L1 << "-way" << endl;
    cout << "L2 Associativity: " << ASSOCIATIVITY_L2 << "-way" << endl;
    cout << "L1 Sets: " << L1_SETS << endl;
    cout << "L2 Sets: " << L2_SETS << endl;
    cout << "L1 Latency: " << L1_LATENCY << " cycles" << endl;
    cout << "L2 Latency: " << L2_LATENCY << " cycles" << endl;
    cout << "Main Memory Latency: " << MAIN_MEMORY_LATENCY << " cycles" << endl;
    cout << "===============================" << endl;
}

void initializeCaches() {
    // Initialize L1 caches (one per core)
    l1_cache.resize(CORE_COUNT);
    for (int core = 0; core < CORE_COUNT; core++) {
        l1_cache[core].resize(L1_SETS);
        for (int set = 0; set < L1_SETS; set++) {
            l1_cache[core][set].resize(ASSOCIATIVITY_L1);
        }
    }
    
    // Initialize shared L2 cache
    l2_cache.resize(L2_SETS);
    for (int set = 0; set < L2_SETS; set++) {
        l2_cache[set].resize(ASSOCIATIVITY_L2);
    }
    
    cout << "Caches initialized successfully" << endl;
}

// Cache addressing functions
uint32_t getOffset(uint32_t address) {
    return address & (LINE_SIZE - 1);
}

uint32_t getL1Index(uint32_t address) {
    return (address >> log2int(LINE_SIZE)) & (L1_SETS - 1);
}

uint32_t getL2Index(uint32_t address) {
    return (address >> log2int(LINE_SIZE)) & (L2_SETS - 1);
}

uint32_t getL1Tag(uint32_t address) {
    return address >> (log2int(LINE_SIZE) + log2int(L1_SETS));
}

uint32_t getL2Tag(uint32_t address) {
    return address >> (log2int(LINE_SIZE) + log2int(L2_SETS));
}

// LRU implementation
int getLRUWay(const vector<CacheLine>& set) {
    int lru_way = 0;
    int max_counter = -1;
    
    // First check for invalid lines (those are preferred)
    for (int way = 0; way < set.size(); way++) {
        if (!set[way].valid) return way;
    }
    
    // If all lines are valid, find the least recently used
    for (int way = 0; way < set.size(); way++) {
        if (set[way].counter > max_counter) {
            max_counter = set[way].counter;
            lru_way = way;
        }
    }
    
    return lru_way;
}

void updateLRU(vector<CacheLine>& set, int accessed_way) {
    // Reset counter for accessed way and increment others
    for (int way = 0; way < set.size(); way++) {
        if (set[way].valid) {
            if (way == accessed_way) {
                set[way].counter = 0;  // Most recently used
            } else {
                set[way].counter++;    // Less recently used
            }
        }
    }
}

// Memory allocation functions
uint32_t allocate_base_address(int core_id) {
    if (core_id < 0 || core_id >= CORE_COUNT) {
        cerr << "Invalid core ID: " << core_id << endl;
        exit(1);
    }
    return heap_pointers[core_id];
}

uint32_t allocate_memory(size_t num_elements, int core_id) {
    uint32_t base_address = allocate_base_address(core_id);
    uint32_t &heap_pointer = heap_pointers[core_id];

    if (heap_pointer + (num_elements * 4) >= base_address + 1024) {
        cerr << "Error: Core " << core_id << " exceeded its 1KB memory limit!\n";
        exit(1);
    }

    uint32_t allocated_address = heap_pointer;
    heap_pointer += num_elements * 4;
    return allocated_address;
}

// Load word with cache implementation
int lw(int address, int core_id) {
    memory_accesses++;
    int cycles_used = 0;

    if (address < 0 || address + 3 >= MEMORY_SIZE || address % 4 != 0) {
        cerr << "Invalid memory read at address " << address << "!\n";
        exit(1);
    }

    uint32_t offset = getOffset(address);
    uint32_t block_start = address - offset;
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getL1Tag(address);

    // Check L1 cache
    auto &l1_set = l1_cache[core_id][l1_idx];
    for (int way = 0; way < ASSOCIATIVITY_L1; way++) {
        if (l1_set[way].valid && l1_set[way].tag == l1_tag) {
            // L1 cache hit
            cache_l1_hits++;
            updateLRU(l1_set, way);
            cycles_used += L1_LATENCY;
            int word;
            memcpy(&word, &l1_set[way].data[offset], sizeof(int));
            stall_cycles += cycles_used;
            return word;
        }
    }
    
    // L1 cache miss
    cache_l1_misses++;
    
    // Try L2 cache
    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getL2Tag(address);
    auto &l2_set = l2_cache[l2_idx];
    
    for (int way = 0; way < ASSOCIATIVITY_L2; way++) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            // L2 cache hit
            cache_l2_hits++;
            updateLRU(l2_set, way);
            
            // Update L1 cache with data from L2
            int l1_replace_way = getLRUWay(l1_set);
            memcpy(l1_set[l1_replace_way].data, l2_set[way].data, LINE_SIZE);
            l1_set[l1_replace_way].tag = l1_tag;
            l1_set[l1_replace_way].valid = true;
            updateLRU(l1_set, l1_replace_way);
            
            int word;
            memcpy(&word, &l1_set[l1_replace_way].data[offset], sizeof(int));
            
            cycles_used += L1_LATENCY + L2_LATENCY;
            stall_cycles += cycles_used;
            return word;
        }
    }
    
    // L2 cache miss, fetch from main memory
    cache_l2_misses++;
    
    // Update L2 cache
    int l2_replace_way = getLRUWay(l2_set);
    memcpy(l2_set[l2_replace_way].data, &memory_main[block_start], LINE_SIZE);
    l2_set[l2_replace_way].tag = l2_tag;
    l2_set[l2_replace_way].valid = true;
    updateLRU(l2_set, l2_replace_way);
    
    // Update L1 cache
    int l1_replace_way = getLRUWay(l1_set);
    memcpy(l1_set[l1_replace_way].data, l2_set[l2_replace_way].data, LINE_SIZE);
    l1_set[l1_replace_way].tag = l1_tag;
    l1_set[l1_replace_way].valid = true;
    updateLRU(l1_set, l1_replace_way);
    
    int word;
    memcpy(&word, &l1_set[l1_replace_way].data[offset], sizeof(int));
    
    cycles_used += L1_LATENCY + L2_LATENCY + MAIN_MEMORY_LATENCY;
    stall_cycles += cycles_used;
    return word;
}

// Store word with cache implementation (write-through policy)
void sw(int address, int value, int core_id) {
    memory_accesses++;
    int cycles_used = 0;

    if (address < 0 || address + 3 >= MEMORY_SIZE) {
        cerr << "Memory write out of bounds at address " << address << "!\n";
        exit(1);
    }

    uint32_t offset = getOffset(address);
    if (offset % 4 != 0) {
        cerr << "Unaligned memory write at address " << address << "!\n";
        exit(1);
    }
    
    // Write to main memory (write-through policy)
    memcpy(&memory_main[address], &value, sizeof(int));
    cycles_used += MAIN_MEMORY_LATENCY;
    
    // Update L1 cache if the block is present
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getL1Tag(address);
    auto &l1_set = l1_cache[core_id][l1_idx];
    
    bool l1_hit = false;
    for (int way = 0; way < ASSOCIATIVITY_L1; way++) {
        if (l1_set[way].valid && l1_set[way].tag == l1_tag) {
            // Update L1 cache
            memcpy(&l1_set[way].data[offset], &value, sizeof(int));
            updateLRU(l1_set, way);
            cache_l1_hits++;
            l1_hit = true;
            cycles_used += L1_LATENCY;
            break;
        }
    }
    
    if (!l1_hit) {
        cache_l1_misses++;
    }
    
    // Update L2 cache if the block is present
    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getL2Tag(address);
    auto &l2_set = l2_cache[l2_idx];
    
    bool l2_hit = false;
    for (int way = 0; way < ASSOCIATIVITY_L2; way++) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            // Update L2 cache
            memcpy(&l2_set[way].data[offset], &value, sizeof(int));
            updateLRU(l2_set, way);
            cache_l2_hits++;
            l2_hit = true;
            cycles_used += L2_LATENCY;
            break;
        }
    }
    
    if (!l2_hit) {
        cache_l2_misses++;
    }
    
    stall_cycles += cycles_used;
    cout << "Stored value: " << value << " at address: " << address << endl;
}

void printMemory(int start_addr = 0, int end_addr = MEMORY_SIZE) {
    cout << "\n=== Memory State ===\n";
    for (int addr = start_addr; addr < end_addr && addr < MEMORY_SIZE; addr += 4) {
        int value;
        memcpy(&value, &memory_main[addr], sizeof(int));
        cout << "0x" << hex << addr << ": " << dec << value << "\t";
        if ((addr / 4 + 1) % 8 == 0) cout << endl;
    }
    cout << endl;
}

void printCacheStats() {
    cout << "\n=== Cache Statistics ===\n";
    cout << "Total Memory Accesses: " << memory_accesses << endl;
    cout << "L1 Cache Hits: " << cache_l1_hits << endl;
    cout << "L1 Cache Misses: " << cache_l1_misses << endl;
    cout << "L2 Cache Hits: " << cache_l2_hits << endl;
    cout << "L2 Cache Misses: " << cache_l2_misses << endl;
    
    // Calculate hit and miss rates
    float l1_hit_rate = (memory_accesses > 0) ? 
                         ((float)cache_l1_hits / memory_accesses * 100) : 0;
    float l1_miss_rate = (memory_accesses > 0) ? 
                          ((float)cache_l1_misses / memory_accesses * 100) : 0;
    float l2_hit_rate = (cache_l1_misses > 0) ? 
                         ((float)cache_l2_hits / cache_l1_misses * 100) : 0;
    float l2_miss_rate = (cache_l1_misses > 0) ? 
                          ((float)cache_l2_misses / cache_l1_misses * 100) : 0;
    
    cout << "L1 Hit Rate: " << fixed << setprecision(2) << l1_hit_rate << "%" << endl;
    cout << "L1 Miss Rate: " << fixed << setprecision(2) << l1_miss_rate << "%" << endl;
    cout << "L2 Hit Rate (of L1 misses): " << fixed << setprecision(2) << l2_hit_rate << "%" << endl;
    cout << "L2 Miss Rate (of L1 misses): " << fixed << setprecision(2) << l2_miss_rate << "%" << endl;
    
    cout << "Total Stall Cycles: " << stall_cycles << endl;
}

// Initialize the memory subsystem
void initializeMemorySubsystem(const string& specs_file) {
    // Read specifications from file
    readSpecifications(specs_file);
    
    // Initialize caches
    initializeCaches();
}
