#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "core.cpp"
using namespace std;
bool stalling;
int global_pc=0;
vector<std::pair<string, vector<string>>> instructions;
unordered_map<string, vector<int>> label_map; // Stores base addresses for each core
unordered_map<string, int> latencies;  // Instruction latencies



void fetchInstruction(vector<Core>& cores) {  // Pass pc as reference
    cout << "Instruction size: " << instructions.size() << endl;
    bool branch_taken = false;
    string instr ;
    vector<string> args;
    for (Core &core : cores) {       
        if (core.pipeline.fetch_stall) {
            std::cout << "[Core " << core.core_id << "] Fetch Stalled, skipping instruction fetch.\n";
            core.pipeline.fetch_stall=true;
            return;
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
            // If this core should not execute the instruction, skip execution
            if (core.core_id == core.cid_val) {
                core.pc = core.global_pc;
                std::cout << "[Core " << core.core_id << "] Branching to PC: " << core.global_pc << "\n";
                core.global_pc = 0;
                branch_taken = true; 
                          
            }
            core.cid_val=-1;
    } 
    if (core.pc >= instructions.size()) {
        std::cout << "No more instructions to fetch\n";
        return;
    }
       // Fetch instruction for this core
        instr = instructions[core.pc].first;
        args =instructions[core.pc].second;
        core.pipeline.IF_ID.instruction = instructions[core.pc].first;
        core.pipeline.IF_ID.args = instructions[core.pc].second;
        core.pipeline.IF_ID.valid_instruction = true;
        core.pipeline.IF_ID.valid_data = false;
        core.pc++;
          
    std::cout <<"[CORE "<<core.core_id<< " ]Fetched instruction: " << instr << " at PC: " << core.pc << "\n";
    
    cout << "PC after fetch: " << core.pc << endl;
    
       
 
}
}

void parseAssembly(const std::string &filename, vector<Core>& cores) {  // Pass cores by reference
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
                    label_map[first][i] = memoryAddress;  // Store per-core base address
                    for (size_t j = 0; j < args.size(); j++) {
                        int value = std::stoi(args[j]);
                        sw1(memoryAddress + (j * 4), value, i);
                    }
                }
            }
        } else if (inTextSection) {
            if(first.empty()){
                break ;
            }
            if (first.back() == ':') {
                first.pop_back();
                label_map[first] = std::vector<int>(CORE_COUNT, instructions.size());
            } 
            else {
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
                instructions.emplace_back(instr, args);
            }
        }
    }
    std::cout << "Labels stored in label_map:\n";
     for (const auto& entry : label_map) {
    std::cout << "Label: " << entry.first << " -> Instruction Index: ";
    for (int core_id = 0; core_id < CORE_COUNT; core_id++) {
        std::cout << "[" << core_id << "] = " << entry.second[core_id] << " ";
    }
   
    std::cout << std::endl;
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

        // 4️⃣ Increment clock cycle for all cores
        for (Core &core : cores) {
            core.clock_cycles++;
           
        }

        // Check if any core is still active
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

void getInstructionLatencies() {
    int num_instructions;
    std::cout << "\nEnter the number of arithmetic instructions with variable latencies: ";
    cin >> num_instructions;

    for (int i = 0; i < num_instructions; i++) {
        std::string instr;
        int latency;
        std::cout << "Enter instruction name and latency: ";
        std::cin >> instr >> latency;
        latencies[instr] = latency;
    }

    std::cout << "\nInstruction Latencies Set:\n";
    for (const auto &entry : latencies) {
        std::cout << entry.first << " -> " << entry.second << " cycles\n";
    }
}

int main() {
    std::vector<Core> cores;
    bool forwarding_option;
    
    std::cout << "Enable data forwarding? (1 = Yes, 0 = No): ";
    std::cin >> forwarding_option;
      parseAssembly("assembly.asm", cores);
      getInstructionLatencies();
    for (int i = 0; i < CORE_COUNT; i++) {
        cores.emplace_back(i, label_map, forwarding_option, latencies);
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
    }

     printCaches();
     printMemory();

    return 0;
}
