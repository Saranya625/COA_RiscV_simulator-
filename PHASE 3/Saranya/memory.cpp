#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include<cmath>
#include<deque>
#include<map>
#include<vector>
#include <cstring>
#include <unordered_map>
#include<iomanip>
using namespace std;
#define MEMORY_SIZE 4096  
#define REGISTER_COUNT 32  
#define INSTRUCTION_MEMORY_SIZE 1024
#define CORE_COUNT 4
#define LINE_SIZE 16
vector<std::pair<string, vector<string>>> instructions;
uint32_t memory_main[MEMORY_SIZE] = {0}; 
int registers[REGISTER_COUNT] = {0};
std::unordered_map<uint32_t, uint32_t> memory_map;
uint32_t heap_pointers[4] = {0, 1024, 2048, 3072};  
int l1_cache_size;
int l2_cache_size;
int associativity_l1;
int associativity_l2;
int l1_latency;
int l2_latency;
string replacement_policy;
int main_memory_latency;
int memory_latency;
int cache_l1_hits[CORE_COUNT]={0};
int cache_l1_misses[CORE_COUNT] ={0};
int cache_l2_hits = 0;
int cache_l2_misses = 0;
int memory_accesses = 0;
void store_instrction(string instr , vector<string> args) {
    instructions.push_back(std::make_pair(instr, args));
}
string fetch_instruction(int pc){
    return instructions[pc].first;
}
vector<string> fetch_args(int pc){
    return instructions[pc].second;
}
int instruction_size(){
    return instructions.size();
}
int L1_SETS;
int L2_SETS;

struct CacheLine {
    bool valid = false;
    uint32_t tag = 0;
    uint8_t line_data[LINE_SIZE] = {0};
    int counter = 0;
};

vector<CacheLine>** l1_cache; // 2D array for each core's L1 cache
vector<CacheLine>* l2_cache;  // 1D array for L2 cache


void initializeCaches() {
    L1_SETS = l1_cache_size / (LINE_SIZE * associativity_l1);
    L2_SETS = l2_cache_size / (LINE_SIZE * associativity_l2);

    l1_cache = new vector<CacheLine>*[CORE_COUNT];
    for (int i = 0; i < CORE_COUNT; i++) {
        l1_cache[i] = new vector<CacheLine>[L1_SETS];
        for (int j = 0; j < L1_SETS; j++) {
            l1_cache[i][j].resize(associativity_l1);
        }
    }

    l2_cache = new vector<CacheLine>[L2_SETS];
    for (int i = 0; i < L2_SETS; i++) {
        l2_cache[i].resize(associativity_l2);
    }
}

void initializeSystem(int l1_size, int l2_size, int l1_assoc, int l2_assoc, int l1_lat, int l2_lat, string repl_policy, int main_mem_lat) {
    l1_cache_size = l1_size;
    l2_cache_size = l2_size;
    associativity_l1 = l1_assoc;
    associativity_l2 = l2_assoc;
    l1_latency = l1_lat;
    l2_latency = l2_lat;
    replacement_policy = repl_policy;
    main_memory_latency = main_mem_lat;
    
    initializeCaches();
}
int log2int(int x) { return static_cast<int>(log2(x)); }
int getSetsL1() { return l1_cache_size / (LINE_SIZE * associativity_l1); }
int getSetsL2() { return l2_cache_size / (LINE_SIZE * associativity_l2); }

int lw(int address) {
    if (address < 0 || address > MEMORY_SIZE - sizeof(int)) {
        throw std::out_of_range("Memory access out of bounds");
    }
    int value;
    std::memcpy(&value, &memory_main[address], sizeof(int));
    
    
    return value;
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
int getLRUWay(vector<CacheLine>& set) {
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
void updateLRU(vector<CacheLine>& set, int accessed_way) {
    for (int way = 0; way < set.size(); ++way) {
        if (way == accessed_way) {
            set[way].counter = 0;
        } else if (set[way].valid) {
            set[way].counter++;
        }
    }
}
uint32_t allocate_base_adress(int core_id) {
    if (core_id < 0 || core_id >= 4) {
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }
    return heap_pointers[core_id];  
}
uint32_t allocate_memory(size_t num_elements, int core_id) {
    if (core_id < 0 || core_id >= 4) {
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }

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
            updateLRU(l1_set, way);
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
            updateLRU(l2_set, way);
            int l1_replace = getLRUWay(l1_set);
             for(int i = 0; i < LINE_SIZE; i++) {
            l1_set[l1_replace].line_data[i] = l2_set[way].line_data[i];
              }
          
           // memccpy(&l1_set[l1_replace].line_data, &l2_set[way].line_data, LINE_SIZE,sizeof(uint32_t));
            updateLRU(l1_set, l1_replace);
            int word;
            memcpy(&word, &l1_set[l1_replace].line_data[offset], sizeof(int));
            return word;
        }
    }
    cache_l2_misses++;
    uint32_t block_start = address - offset;
    int l1 = getLRUWay(l1_set);   
    for(int i = 0; i < LINE_SIZE; i++) {
        l1_set[l1].line_data[i] = lw(block_start + i);
    }
    l1_set[l1].tag = l1_tag;
    l1_set[l1].valid = true;
    updateLRU(l1_set, l1);
    int l2_replace = getLRUWay(l2_set);
    for (int i = 0; i <LINE_SIZE; i++) {
    l2_set[l2_replace].line_data[i] = lw(block_start + i);
    }
    l2_set[l2_replace].tag = l2_tag;
    l2_set[l2_replace].valid = true;
    updateLRU(l2_set, l2_replace);

    /*int l1_replace = 0;
    l1_set[l1_replace] = l2_set[l2_replace];
    updateLRU(l1_set, l1_replace);*/
    memory_latency= main_memory_latency;
    int word;
    memcpy(&word, &l1_set[l1].line_data[offset], sizeof(int));
    return word;
}
void sw(int address, int value, int core_id) {
    int base_address = core_id * 1024;
    int max_address = base_address + 1024;

    if (address < 0 || address + 3 >= MEMORY_SIZE || address % 4 != 0) {
        std::cerr << "Error: Core " << core_id << " tried to write to invalid memory address!\n";
        exit(1);
    }

    // Step 1: Write to main memory
    std::memcpy(&memory_main[address], &value, sizeof(int));
    memory_latency = main_memory_latency;

    // Step 2: Update L1 if block is present
    uint32_t offset = getOffset(address);
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getTag_L1(address);
    auto& l1_set = l1_cache[core_id][l1_idx];
    if (l1_set.empty()) l1_set.resize(associativity_l1);

    for (int way = 0; way < associativity_l1; ++way) {
        if (l1_set[way].valid && l1_set[way].tag == l1_tag) {
            std::memcpy(&l1_set[way].line_data[offset], &value, sizeof(int));
            updateLRU(l1_set, way);
            memory_latency = l1_latency;
            break;  // Don't return yet, continue to check L2
        }
    }

    // Step 3: Optionally update L2 (if block exists)
    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    auto& l2_set = l2_cache[l2_idx];
    if (l2_set.empty()) l2_set.resize(associativity_l2);

    for (int way = 0; way < associativity_l2; ++way) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            std::memcpy(&l2_set[way].line_data[offset], &value, sizeof(int));
            updateLRU(l2_set, way);
            // Optional: memory_latency = l2_latency;
            break;
        }
    }
}





void sw1(int address, int value, int core_id) {
    int base_address = (core_id) * 1024;
    int max_address = base_address + 1024;
    if (address < 0 || address >= MEMORY_SIZE) {
        std::cerr << "Error: Core " << core_id << " tried to access memory out of its range!\n";
        exit(1);
    }
     
    int old_value = lw(address);
    
    std::memcpy(&memory_main[address], &value, sizeof(int));
    int new_value = lw(address);
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
        cout<< "Hit Rate L1: " << (float)cache_l1_hits[i]/(cache_l1_hits[i]+cache_l1_misses[i]) * 100 << "%" << endl;
    }
    cout<< "Hit Rate L2: " << (float)cache_l2_hits/(memory_accesses) * 100 << "%" << endl;
   for(int i=0; i<CORE_COUNT; i++){
        cout<< "Miss Rate L1: " << (float)cache_l1_misses[i]/(cache_l1_hits[i]+cache_l1_misses[i]) * 100 << "%" << endl;
    }
    cout<< "Miss Rate L2: " << (float)cache_l2_misses/(memory_accesses) * 100 << "%" << endl;
}
void printCaches() {
    cout << "        L1 Cache         \n";
    cout << "=========================\n";

    for (int core = 0; core < CORE_COUNT; ++core) {
        cout << "Core " << core << ":\n";
        for (int set = 0; set < L1_SETS; ++set) {
            cout << "  Set " << set << ":\n";
            for (int way = 0; way < l1_cache[core][set].size(); ++way) {
                const CacheLine& line = l1_cache[core][set][way];
                cout << " Way " << way << "|Valid: " << line.valid
                     << " |Tag: 0x" << hex << line.tag << dec
                     << " |Counter: " << line.counter
                     << " |Data: ";
                for (size_t i = 0; i < LINE_SIZE; i += 4) {
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
        for (int way = 0; way < l2_cache[set].size(); ++way) {
            const CacheLine& line = l2_cache[set][way];
            cout << "  Way " << way << " | Valid: " << line.valid
                 << " | Tag: 0x" << hex << line.tag << dec
                 << " | Counter: " << line.counter
                 << " | Data: ";
             for (size_t i = 0; i < LINE_SIZE; i += 4) {
             cout << dec << static_cast<int>(line.line_data[i]) << " ";
         }

            cout << dec << "\n";
        }
        cout << "\n";
    }
}

class SP_memory {
public:
    int size_sp;
    int latency_sp;
    int associativity_sp;
    std::vector<uint8_t> sp_memory;

    SP_memory(int size, int latency, int associativity)
        : size_sp(size), latency_sp(latency), associativity_sp(associativity), sp_memory(size, 0) {}

    int lw_spm(int address) {
        int value;
        std::memcpy(&value, &sp_memory[address], sizeof(int));
        return value;
    }

    void sw_spm(int address, int value) {

        sp_memory[address] = value;
        std::memcpy(&sp_memory[address], &value, sizeof(int));
    }
    void sp_printMemory() {
    std::cout << "\n=== ScratchPad Memory State ===\n";
    for (int addr = 0; addr < size_sp; addr += 4) {
        int value = sp_memory[addr];
        std::cout << "0x" << std::hex << addr << ": " << std::dec << value << "\t";
        if ((addr / 4 + 1) % 8 == 0) std::cout << std::endl;  
    }
    std::cout << std::endl;
}

};