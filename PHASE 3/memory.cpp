#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include<cmath>
#include<deque>
#include <climits>
#include<map>
#include<vector>
#include <cstring>
#include <string>
#include <sstream>
#include <unordered_map>
#include<iomanip>
#include <algorithm>

using namespace std;
#define MEMORY_SIZE 4096  
#define REGISTER_COUNT 32  
#define CORE_COUNT 4
#define INSTRUCTION_MEMORY_SIZE 1024
vector<std::pair<string, vector<string>>> instructions;
uint32_t memory_main[MEMORY_SIZE] = {0}; 
vector<string>instruction_memory {INSTRUCTION_MEMORY_SIZE,"0"};
int registers[REGISTER_COUNT] = {0};
std::unordered_map<uint32_t, uint32_t> memory_map;
uint32_t heap_pointers[4] = {0, 1024, 2048, 3072};  
int line_size = 0;
int l1_cache_size;
int l2_cache_size;
int associativity_l1;
int associativity_l2;
int l1_latency;
int l2_latency;
string replacement_policy;
int main_memory_latency;
int memory_latency; //memory latency 
int fetch_latency;
int cache_l1_hits[CORE_COUNT]={0};
int cache_l1_misses[CORE_COUNT] ={0};
int cache_l2_hits = 0;
int cache_l2_misses = 0;
int instruction_cache_l1_hits[CORE_COUNT]={0};
int instruction_cache_l1_misses[CORE_COUNT]={0};
int instruction_cache_l2_hits = 0;
int instruction_cache_l2_misses = 0;
int memory_accesses = 0;
int instruction_memory_accesses = 0;
int L1_SETS;
int L2_SETS;

struct ICacheLine {
    bool valid = false;         
    uint32_t tag = 0;            
    int pc = -1;                
    vector<string> instruction ;     
    int counter = 0;        
};
vector<ICacheLine>** L1I_cache;
vector<ICacheLine>* L2I_cache;
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
void init_instruction_caches() {
    L1I_cache = new vector<ICacheLine>*[CORE_COUNT];
    for (int i = 0; i < CORE_COUNT; ++i) {
        L1I_cache[i] = new vector<ICacheLine>[L1_SETS];
        for (int j = 0; j < L1_SETS; ++j){
            L1I_cache[i][j].resize(associativity_l1);
            for(int k = 0; k < associativity_l1; ++k) {
                L1I_cache[i][j][k].instruction.resize(line_size,"0"); // Initialize with a default value
            }
        }          
    }

    L2I_cache = new vector<ICacheLine>[L2_SETS];
    for (int j = 0; j < L2_SETS; ++j){
        L2I_cache[j].resize(associativity_l2);
        for(int k = 0; k < associativity_l2; ++k) {
            L2I_cache[j][k].instruction.resize(line_size,"0"); // Initialize with a default value
        }
    }
        
}
int getLRUWay_I(vector<ICacheLine>& set) {
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
void updateLRU_I(vector<ICacheLine>& set, int accessed_way) {
    for (int way = 0; way < set.size(); ++way) {
        if (way == accessed_way) {
            set[way].counter = 0;
        } else if (set[way].valid) {
            set[way].counter++;
        }
    }
}
int getMRUWay_I(vector<ICacheLine>& set) {
    int mru_way = -1;
    int min_counter = -1;
    for (int way = 0; way < set.size(); ++way) {
        if (!set[way].valid) return way; // Prefer invalid (empty) lines first
        if (set[way].counter > min_counter) {
            min_counter = set[way].counter;
            mru_way = way;
        }
    }
    return mru_way; // Return the most recently used valid way
}
void updateMRU_I(vector<ICacheLine>& set, int accessed_way) {
    int max_counter = 0;
    for (const auto& line : set) {
        if (line.valid && line.counter > max_counter) {
            max_counter = line.counter;
        }
    }
    set[accessed_way].counter = max_counter + 1;
}
int get_replacement_way_I(vector<ICacheLine>& set) {
    if (replacement_policy == "LRU") {
        return getLRUWay_I(set);
    } else if (replacement_policy == "MRU") {
        return getMRUWay_I(set);
    } else {
        std::cerr << "Invalid replacement policy: " << replacement_policy << std::endl;
        exit(1);
    }
}
void updateReplacement_I(vector<ICacheLine>& set, int accessed_way) {
    if (replacement_policy == "LRU") {
        updateLRU_I(set, accessed_way);
    } else if (replacement_policy == "MRU") {
        updateMRU_I(set, accessed_way);
    } else {
        std::cerr << "Invalid replacement policy: " << replacement_policy << std::endl;
        exit(1);
    }
}



void store_instruction(string instr , vector<string> args) {
    instructions.push_back(std::make_pair(instr, args));
    int pc= (instructions.size() - 1)*4;  
    instruction_memory[pc] = instr;
}
void print_instructions(){
    cout<<"instructions"<<endl;
    for(int i=0;i<instructions.size();i++){
        cout<<instructions[i].first<<endl;
    }
}
void print_instruction_memory(const vector<string>& instruction_memory) {
    for (int pc = 0; pc <INSTRUCTION_MEMORY_SIZE; pc=pc+4) {
        cout << "PC " << pc << ": " << instruction_memory[pc] << " ";
    }
    
}

string fetch_instruction(int pc, int core_id) {
    instruction_memory_accesses++;
    int pc_mod=pc*4;
    int set_index = getL1Index(pc_mod);
    uint32_t tag = getTag_L1(pc_mod);
    int offset = getOffset(pc_mod); // Should be in range 0 to line_size-1

    auto& l1_set = L1I_cache[core_id][set_index];
    if (l1_set.empty()) l1_set.resize(associativity_l1);

    for (int way = 0; way < associativity_l1; way++) {
        if (l1_set[way].valid && l1_set[way].tag == tag) {
            fetch_latency= l1_latency;
            instruction_cache_l1_hits[core_id]++;
            updateReplacement_I(l1_set, way);
            return l1_set[way].instruction[offset];
        }
    }

    instruction_cache_l1_misses[core_id]++;

    int l2_set_index = getL2Index(pc_mod);
    uint32_t l2_tag = getTag_L2(pc_mod);
    auto& l2_set = L2I_cache[l2_set_index];
    if (l2_set.empty()) l2_set.resize(associativity_l2);

    for (int way = 0; way < associativity_l2; way++) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            instruction_cache_l2_hits++;
            fetch_latency= l2_latency;
            int victim = get_replacement_way_I(l1_set);
            l1_set[victim].valid = true;
            l1_set[victim].tag = tag;
            for (int i = 0; i < line_size; i++) {
                l1_set[victim].instruction[i] = l2_set[way].instruction[i];
            }
            updateReplacement_I(l1_set, victim);

            return l2_set[way].instruction[offset];
        }
    }


    instruction_cache_l2_misses++;

    vector<string> fetched_block(line_size, "");  // Each block holds `line_size` instructions
    int block_start_pc = pc_mod - offset;
    for (int i = 0; i < line_size; i=i+4) {
        int fetch_pc = block_start_pc + i;
        if (fetch_pc / 4 < instructions.size()) {
            fetched_block[i] = instructions[fetch_pc / 4].first;
        } else {
            fetched_block[i] = "NOP";  // Or any default instruction
        }
    }
    fetch_latency=main_memory_latency;
    int l2_victim = get_replacement_way_I(l2_set);
    l2_set[l2_victim].valid = true;
    l2_set[l2_victim].tag = l2_tag;
    l2_set[l2_victim].instruction = fetched_block;
    updateReplacement_I(l2_set, l2_victim);

    // Fill L1 with same block
    int l1_victim = get_replacement_way_I(l1_set);
    l1_set[l1_victim].valid = true;
    l1_set[l1_victim].tag = tag;
    l1_set[l1_victim].instruction = fetched_block;
    updateReplacement_I(l1_set, l1_victim);
    return instruction_memory[pc_mod]; // Return the instruction at the given PC
}


vector<string> fetch_args(int pc){
    return instructions[pc].second;
}
int instruction_size(){
    return instructions.size();
}



struct CacheLine {
    bool valid = false;
    uint32_t tag = 0;
    vector<uint8_t> line_data ;
    int counter = 0;
};
vector<CacheLine>** l1_cache; 
vector<CacheLine>* l2_cache; 
void initializeCaches() {
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
       // l2I_cache[i].resize(associativity_l2);
          for (int j = 0; j < associativity_l2; j++) {
            l2_cache[i][j].line_data.resize(line_size,0);  // ✅ Resize line_data
            //l2I_cache[i][j].line_data.resize(line_size,0);
        }
    }
}
void initializeSystem(int line_size1 , int l1_size, int l2_size, int l1_assoc, int l2_assoc, int l1_lat, int l2_lat, string repl_policy, int main_mem_lat) {
    line_size = line_size1;
    l1_cache_size = l1_size;
    l2_cache_size = l2_size;
    associativity_l1 = l1_assoc;
    associativity_l2 = l2_assoc;
    l1_latency = l1_lat;
    l2_latency = l2_lat;
    replacement_policy = repl_policy;
    main_memory_latency = main_mem_lat;
    
    initializeCaches();
    init_instruction_caches();
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
int getMRUWay(vector<CacheLine>& set) {
    int mru_way = -1;
    int min_counter = -1;
    for (int way = 0; way < set.size(); ++way) {
        if (!set[way].valid) return way; // Prefer invalid (empty) lines first
        if (set[way].counter > min_counter) {
            min_counter = set[way].counter;
            mru_way = way;
        }
    }
    return mru_way; // Return the most recently used valid way
}
void updateMRU(vector<CacheLine>& set, int accessed_way) {
    int max_counter = 0;
    for (const auto& line : set) {
        if (line.valid && line.counter > max_counter) {
            max_counter = line.counter;
        }
    }
    set[accessed_way].counter = max_counter + 1;
}
int get_replacement_way(vector<CacheLine>& set) {
    if (replacement_policy == "LRU") {
        return getLRUWay(set);
    } else if (replacement_policy == "MRU") {
        return getMRUWay(set);
    } else {
        std::cerr << "Invalid replacement policy: " << replacement_policy << std::endl;
        exit(1);
    }
}
void updateReplacement(vector<CacheLine>& set, int accessed_way) {
    if (replacement_policy == "LRU") {
        updateLRU(set, accessed_way);
    } else if (replacement_policy == "MRU") {
        updateMRU(set, accessed_way);
    } else {
        std::cerr << "Invalid replacement policy: " << replacement_policy << std::endl;
        exit(1);
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
          
           // memccpy(&l1_set[l1_replace].line_data, &l2_set[way].line_data, line_size,sizeof(uint32_t));
            updateReplacement(l1_set, l1_replace);
            int word;
            memcpy(&word, &l1_set[l1_replace].line_data[offset], sizeof(int));
            return word;
        }
    }
    cache_l2_misses++;
    uint32_t block_start = address - offset;
    int l1 = get_replacement_way(l1_set);   
    for(int i = 0; i < line_size; i++) {
        l1_set[l1].line_data[i] = lw(block_start + i);
    }
    l1_set[l1].tag = l1_tag;
    l1_set[l1].valid = true;
    updateReplacement(l1_set, l1);
    int l2_replace = get_replacement_way(l2_set);
    for (int i = 0; i <line_size; i++) {
    l2_set[l2_replace].line_data[i] = lw(block_start + i);
    }
    l2_set[l2_replace].tag = l2_tag;
    l2_set[l2_replace].valid = true;
    updateReplacement(l2_set, l2_replace);

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

    uint32_t offset = getOffset(address);
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getTag_L1(address);
    auto& l1_set = l1_cache[core_id][l1_idx];
    if (l1_set.empty()) l1_set.resize(associativity_l1);

    for (int way = 0; way < associativity_l1; ++way) {
        if (l1_set[way].valid && l1_set[way].tag == l1_tag) {
            std::memcpy(&l1_set[way].line_data[offset], &value, sizeof(int));
            updateReplacement(l1_set, way);
            break;  
        }
    }
    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    auto& l2_set = l2_cache[l2_idx];
    if (l2_set.empty()) l2_set.resize(associativity_l2);

    for (int way = 0; way < associativity_l2; ++way) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            std::memcpy(&l2_set[way].line_data[offset], &value, sizeof(int));
            updateReplacement(l2_set, way);
            break;
        }
    }
    std::memcpy(&memory_main[address], &value, sizeof(int));
    memory_latency = main_memory_latency;
     uint32_t block_start = address - offset;
    int l1 = get_replacement_way(l1_set);   
    for(int i = 0; i < line_size; i++) {
        l1_set[l1].line_data[i] = lw(block_start + i);
    }
    l1_set[l1].tag = l1_tag;
    l1_set[l1].valid = true;
    updateReplacement(l1_set, l1);
    int l2_replace = get_replacement_way(l2_set);
    for (int i = 0; i <line_size; i++) {
    l2_set[l2_replace].line_data[i] = lw(block_start + i);
    }
    l2_set[l2_replace].tag = l2_tag;
    l2_set[l2_replace].valid = true;
    updateReplacement(l2_set, l2_replace);

}
void printInstructionCache() {
    cout << "========== L1 Instruction Cache ==========\n";
    for (int core = 0; core < CORE_COUNT; ++core) {
        cout << "Core " << core << ":\n";
        for (int set = 0; set < L1_SETS; ++set) {
            auto& l1_set = L1I_cache[core][set];
            for (int way = 0; way < l1_set.size(); ++way) {
                const auto& block = l1_set[way];
                cout << "  Set " << set << ", Way " << way 
                     << " | Valid: " << block.valid 
                     << " | Tag: " << block.tag 
                     << " | Instructions: ";
                for (int i = 0; i < line_size; i=i+4) {
                    cout << "[" << i << "]:" << block.instruction[i] << " ";
                }
                cout << "\n";
            }
        }
    }

    cout << "\n========== L2 Instruction Cache ==========\n";
    for (int set = 0; set < L2_SETS; ++set) {
        auto& l2_set = L2I_cache[set];
        for (int way = 0; way < l2_set.size(); ++way) {
            const auto& block = l2_set[way];
            cout << "  Set " << set << ", Way " << way 
                 << " | Valid: " << block.valid 
                 << " | Tag: " << block.tag 
                 << " | Instructions: ";
            for (int i = 0; i < line_size; i=i+4) {
                cout << "[" << i << "]:" << block.instruction[i] << " ";
            }
            cout << "\n";
        }
    }
    cout << "==========================================\n";
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
                for (size_t i = 0; i < line_size; i += 4) {
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
             for (size_t i = 0; i < line_size; i += 4) {
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