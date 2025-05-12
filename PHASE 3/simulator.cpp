#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include<algorithm>
#include <string>
#include "core.cpp"
using namespace std;
bool stalling;
int global_pc=0;

unordered_map<string, vector<int>> label_map;  
bool sync_complete = false;
bool check_sync_complete(vector<Core>& cores , int traget_address) {
  for(Core &core : cores) {
    if (core.global_pc != traget_address) {
      return false;
    }
    return true;
  }
}
struct Specifications {
    bool data_forwarding;
    string replacement_policy;
    int line_size;
    int L1_cache_size;
    int L2_cache_size;
    int L1_associativity;
    int L2_associativity;
    int L1_latency;
    int L2_latency;
    int main_memory_latency;
    std::unordered_map<std::string, int> latencies;
};

Specifications parseSpecifications(const std::string &filename) {
    Specifications specs{};
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open " << filename << std::endl;
        exit(1);
    }

    std::string line;
    bool in_latency_block = false;

    while (getline(file, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

        // Skip empty or comment lines
        if (line.empty() || line[0] == '#') continue;

     if (line == "[Arithmeticvariablelatencies:") {
            in_latency_block = true;
            continue;
        }

        // Detect end of latency block
        if (line == "]") {
            in_latency_block = false;
            continue;
        }

        if (in_latency_block) {
            // Parse instruction=latency
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string instr = line.substr(0, eq);
                int latency = std::stoi(line.substr(eq + 1));
                specs.latencies[instr] = latency;
            }
            continue;
        }

        // Parse regular key:value pairs
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        if (key == "data_forwarding") specs.data_forwarding = std::stoi(value);
        else if (key == "replacement_policy") specs.replacement_policy = value;
        else if(key == "line_size") specs.line_size = std::stoi(value);
        else if (key == "L1_cache_size") specs.L1_cache_size = std::stoi(value);
        else if (key == "L2_cache_size") specs.L2_cache_size = std::stoi(value);
        else if (key == "L1_associativity") specs.L1_associativity = std::stoi(value);
        else if (key == "L2_associativity") specs.L2_associativity = std::stoi(value);
        else if (key == "L1_latency") specs.L1_latency = std::stoi(value);
        else if (key == "L2_latency") specs.L2_latency = std::stoi(value);
        else if (key == "Main_memory") specs.main_memory_latency = std::stoi(value);
    }
    return specs;
}
bool parallel_sync(vector<Core>& cores){
    for(int i=0;i<cores.size();i++){
        if(cores[0].pc == cores[i].pc){
            cores[i].fetch_latency = cores[0].fetch_latency;
        }

    }
}

void fetchInstruction(vector<Core>& cores) {  
    cout << "Instruction size: " << instruction_size() << endl;
    bool branch_taken = false;

    string instr ;
    vector<string> args;
  
    for (Core &core : cores) {      
        if (core.pipeline.fetch_stall) {
            std::cout << "[Core " << core.core_id << "] Fetch Stalled, skipping instruction fetch.\n";
            core.pipeline.fetch_stall=true;
            return;
        }
        if(core.barrier_sync == true){
            if(core.global_pc != 0) {
                if (core.cid_val == -1) {
                std::cout << "[Core " << core.core_id << "] Branching to PC: " << core.global_pc << "\n";
                core.pc = core.global_pc;
                core.global_pc = 0;
                branch_taken = true;
                core.pipeline.ID_EX.valid_instruction=false;
            }
            if(core.core_id == core.cid_val) {
                core.pc = core.global_pc;
                if(check_sync_complete(cores, core.global_pc)){
                    core.pipeline.IF_ID.barrier_stall = false;
                    core.cid_val = -1;
                    core.global_pc = 0;

                }
                core.pipeline.ID_EX.barrier_stall = true;                       
            }

        }
    }        
        if (core.global_pc != 0) {
            cout<<"branch started.."<<endl;
            if (core.cid_val == -1) {
                std::cout << "[Core " << core.core_id << "] Branching to PC: " << core.global_pc << "\n";
                core.pc = core.global_pc;
                core.global_pc = 0;
                branch_taken = true;
                core.pipeline.ID_EX.valid_instruction=false;
            }
            if (core.core_id == core.cid_val) {
                core.pc = core.global_pc;
                std::cout << "[Core " << core.core_id << "] Branching to PC: " << core.global_pc << "\n";
                core.global_pc = 0;
                branch_taken = true; 
                          
            }
            core.cid_val=-1;
    }  
        if (core.pc >= instruction_size() ) {
        if(core.pipeline.IF_ID.valid_instruction | core.pipeline.ID_EX.valid_instruction | core.pipeline.EX_MEM.valid_instruction | core.pipeline.MEM_WB.valid_instruction | core.pipeline.WB_Return.valid_instruction){
        std::cout << "No more instructions to fetch\n";
        core.no_instructions_fetch=true;
        }
        else{
            core.no_instructions=true;
        }
    }
        if(!core.no_instructions_fetch){
        core.no_instructions=false;
        args = fetch_args(core.pc);
        instr = fetch_instruction(core.pc,core.core_id);
        core.pipeline.IF_ID.instruction = instr ;
        core.pipeline.IF_ID.args = args;
        core.pipeline.IF_ID.valid_instruction = true;
        core.pipeline.IF_ID.valid_data = false;
        core.pc++;
        core.fetch_latency=fetch_latency;
        parallel_sync(cores);
        cout<<"fetch latency of core "<< core.core_id<<"is"<<core.fetch_latency<<endl;

     }
          
    }
   
}

void parseAssembly(const std::string &filename, vector<Core>& cores) { 
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }

    std::string line;
    bool inTextSection = false, inDataSection = false;
    int instructionIndex = 0;

    while (std::getline(file, line)) {
        size_t commentPos = line.find("#");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string first, instr;
        iss >> first;

        if (first == ".data") {
            inTextSection = false;
            inDataSection = true;
            continue;
        } else if (first == ".text") {
            inTextSection = true;
            inDataSection = false;
            continue;
        }

        if (inDataSection) {
            if (first.back() == ':') {
                first.pop_back();
                label_map[first] = std::vector<int>(CORE_COUNT, 0);
                if (!(iss >> instr)) continue;
            } else {
                instr = first;
            }

            std::vector<std::string> args;
            std::string arg;
            while (std::getline(iss, arg, ',')) {
                arg.erase(0, arg.find_first_not_of(" \t"));
                arg.erase(arg.find_last_not_of(" \t") + 1);
                args.push_back(arg);
            }

            if (instr == ".word") {
                int size = args.size();
                for (int i = 0; i < CORE_COUNT; i++) {
                    int memoryAddress = allocate_memory(size * 4, i);
                    cout<<"Memory Address: " << memoryAddress << endl;
                    label_map[first][i] = memoryAddress;  // Store per-core base address
                    for (size_t j = 0; j < args.size(); j++) {
                        int value = std::stoi(args[j]);
                        sw1(memoryAddress + (j * 4), value, i);
                    }
                }
            }
        } else if (inTextSection) {
            if (first.back() == ':') {
                first.pop_back();
                label_map[first] = std::vector<int>(CORE_COUNT, instruction_size());
            } else {
                instr = first;
                std::vector<std::string> args;
                std::string restOfLine;
                std::getline(iss, restOfLine);
                std::istringstream argStream(restOfLine);
                std::string token;
                while (std::getline(argStream, token, ',')) {
                    token.erase(0, token.find_first_not_of(" \t"));
                    token.erase(token.find_last_not_of(" \t") + 1);
                    args.push_back(token);
                }
                store_instruction(instr, args);
    
            }
        }
    }
    
}

void executePipeline(vector<Core>& cores) {
    std::cout << "Running pipeline for all cores with a shared fetch unit...\n";
    bool running = true;
    int pc = 0;  

    while (running) {
  
        std::cout << "\n====== [Clock Cycle: " << cores[0].clock_cycles + 1 << "] ======" << std::endl;
        bool fetch_stall = false; 
        for (Core &core : cores) {
            core.writeBack();
            core.memoryStage();
            core.executeStage();
            core.decodeInstruction();
            cout<<"/////////////"<<endl;
        }
         fetchInstruction(cores);
        for (Core &core : cores) {
            core.pipeline.shiftStages();
        }

        for (Core &core : cores) {
            if(!core.no_instructions ){
                core.clock_cycles++;
            }
             else if(core.no_instructions && core.barrier_sync){
                core.clock_cycles++;
                core.pipeline.stalls++;
            }

            
        }

        running = false;
        for (Core &core : cores) {
            if (core.pipeline.IF_ID.valid_instruction || core.pipeline.ID_EX.valid_instruction || 
                core.pipeline.EX_MEM.valid_instruction || core.pipeline.MEM_WB.valid_instruction || 
                core.pipeline.WB_Return.valid_instruction) {
                running = true;
                break;
            }
         
        }
        
    }
}



int main() {
    std::vector<Core> cores;
    bool forwarding_option;
    Specifications specs = parseSpecifications("specifications.txt");
    SP_memory spm(specs.L1_cache_size, specs.L1_latency, specs.L1_associativity);

    initializeSystem(specs.line_size, specs.L1_cache_size, specs.L2_cache_size,  specs.L1_associativity, specs.L2_associativity, specs.L1_latency, specs.L2_latency, specs.replacement_policy,specs.main_memory_latency);
    
    forwarding_option = specs.data_forwarding;
    cout << "Data Forwarding: " << (forwarding_option ? "Enabled" : "Disabled") << endl;
    auto latencies = specs.latencies;
    cout<< "Latencies: " << endl;
    for (const auto &entry : latencies) {
        std::cout << entry.first << " -> " << entry.second << " cycles\n";
    }
    parseAssembly("assembly.asm", cores);
    for (int i = 0; i < CORE_COUNT; i++) {
        cores.emplace_back(i, label_map, forwarding_option,latencies,spm,specs.main_memory_latency);
    }
    executePipeline(cores);

   
   for (Core &core : cores) {
        std::cout << "\n====== Results for Core " << core.core_id << " ======\n";
        core.printRegisters();
        std::cout << "Clock Cycles: " << core.clock_cycles << std::endl;
        std::cout << "Stalls: " << core.pipeline.stalls << std::endl;
        std::cout << "Number of instructions executed: " << core.number_instructions << std::endl;
        
        double IPC = (double)core.number_instructions / core.clock_cycles;
        std::cout << "IPC: " << IPC << "\n";
        //core.spm.sp_printMemory();
    }
    //printInstructionCache();
    //printCaches();
    //printMemory();
   

    return 0;
}