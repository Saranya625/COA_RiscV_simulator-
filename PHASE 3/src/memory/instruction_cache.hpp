#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include "../common/constants.hpp"

struct ICacheLine {
    bool valid = false;
    uint32_t tag = 0;
    int pc = -1;
    std::vector<std::string> instruction;
    int counter = 0;
};

extern std::vector<std::pair<std::string, std::vector<std::string>>> instructions;
extern std::vector<std::string> instruction_memory;

extern std::vector<ICacheLine>** L1I_cache;
extern std::vector<ICacheLine>* L2I_cache;

extern int instruction_cache_l1_hits[CORE_COUNT];
extern int instruction_cache_l1_misses[CORE_COUNT];
extern int instruction_cache_l2_hits;
extern int instruction_cache_l2_misses;
extern int instruction_memory_accesses;

void init_instruction_caches();
void store_instruction(std::string instr, std::vector<std::string> args);
std::string fetch_instruction(int pc, int core_id);
std::vector<std::string> fetch_args(int pc);
int instruction_size();

void print_instructions();
void print_instruction_memory(const std::vector<std::string>& instruction_memory);
void printInstructionCache();
