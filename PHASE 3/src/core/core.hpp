#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../common/constants.hpp"
#include "../pipeline/pipeline.hpp"
#include "../memory/memory.hpp"

class Core {
public:
    int global_pc=0;
    const int core_id;
    int pc=0;
    int cid_val=-1;
    int registers[REGISTER_COUNT]={0};
    std::unordered_map<int, int> visited_pcs;
    std::unordered_map<std::string, std::vector<int>> &label_map; 
    std::unordered_map<std::string,int> instruction_latencies; 
    int base_memory_adress;
    Pipeline pipeline;
    int clock_cycles = 0;   
    bool enable_forwarding;  
    int latest_ex_result ;
    int hazard_in_id = 0;
    int number_instructions = 0;
    int base_adress;
    bool barrier_sync=false;
    bool no_instructions_fetch=false;
    bool no_instructions=false;
    SP_memory spm;
    int main_memory_latency;
    int fetch_latency;

    Core(int id, std::unordered_map<std::string,std::vector<int>> &labels, bool forwarding,
         std::unordered_map<std::string,int> latencies, const SP_memory& spm_config, int main_mem_lat);

    int getRegisterIndex(const std::string &reg);

    void decodeInstruction();
    void executeStage();
    void memoryStage();
    void writeBack();
    void printRegisters();
};
