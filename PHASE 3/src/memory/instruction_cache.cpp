#include "instruction_cache.hpp"
#include "memory_config.hpp"
#include "cache_replacement.hpp"
#include <iostream>

using namespace std;

vector<std::pair<string, vector<string>>> instructions;
vector<string> instruction_memory(INSTRUCTION_MEMORY_SIZE, "0");

vector<ICacheLine>** L1I_cache = nullptr;
vector<ICacheLine>* L2I_cache = nullptr;

int instruction_cache_l1_hits[CORE_COUNT] = {0};
int instruction_cache_l1_misses[CORE_COUNT] = {0};
int instruction_cache_l2_hits = 0;
int instruction_cache_l2_misses = 0;
int instruction_memory_accesses = 0;

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

void store_instruction(string instr , vector<string> args) {
    instructions.push_back(std::make_pair(instr, args));
    int pc= (instructions.size() - 1)*4;  
    instruction_memory[pc] = instr;
}

void print_instructions(){
    cout<<"instructions"<<endl;
    for(size_t i=0;i<instructions.size();i++){
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
            updateReplacement(l1_set, way);
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
            int victim = get_replacement_way(l1_set);
            l1_set[victim].valid = true;
            l1_set[victim].tag = tag;
            for (int i = 0; i < line_size; i++) {
                l1_set[victim].instruction[i] = l2_set[way].instruction[i];
            }
            updateReplacement(l1_set, victim);

            return l2_set[way].instruction[offset];
        }
    }


    instruction_cache_l2_misses++;

    vector<string> fetched_block(line_size, "");  // Each block holds `line_size` instructions
    int block_start_pc = pc_mod - offset;
    for (int i = 0; i < line_size; i=i+4) {
        int fetch_pc = block_start_pc + i;
        if (fetch_pc / 4 < (int)instructions.size()) {
            fetched_block[i] = instructions[fetch_pc / 4].first;
        } else {
            fetched_block[i] = "NOP";  // Or any default instruction
        }
    }
    fetch_latency=main_memory_latency;
    int l2_victim = get_replacement_way(l2_set);
    l2_set[l2_victim].valid = true;
    l2_set[l2_victim].tag = l2_tag;
    l2_set[l2_victim].instruction = fetched_block;
    updateReplacement(l2_set, l2_victim);

    // Fill L1 with same block
    int l1_victim = get_replacement_way(l1_set);
    l1_set[l1_victim].valid = true;
    l1_set[l1_victim].tag = tag;
    l1_set[l1_victim].instruction = fetched_block;
    updateReplacement(l1_set, l1_victim);
    return instruction_memory[pc_mod]; // Return the instruction at the given PC
}

vector<string> fetch_args(int pc){
    return instructions[pc].second;
}

int instruction_size(){
    return instructions.size();
}

void printInstructionCache() {
    cout << "========== L1 Instruction Cache ==========\n";
    for (int core = 0; core < CORE_COUNT; ++core) {
        cout << "Core " << core << ":\n";
        for (int set = 0; set < L1_SETS; ++set) {
            auto& l1_set = L1I_cache[core][set];
            for (size_t way = 0; way < l1_set.size(); ++way) {
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
        for (size_t way = 0; way < l2_set.size(); ++way) {
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
