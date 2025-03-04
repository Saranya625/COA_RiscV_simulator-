#include <iostream>
#include <vector>
#include <unordered_map>
#include "memory.cpp"


#define CORE_COUNT 4
#define REGISTER_COUNT 32


struct PipelineStage {
    std::string instruction;
    std::vector<std::string> args;
    int result;
    int rs1, rs2, rd;
    bool valid;
    bool hazard_detected;
    bool hazard_from_EX;
    bool hazard_from_MEM;
    PipelineStage() : valid(false), result(0), rs1(0), rs2(0), rd(0),
    hazard_detected(false), hazard_from_EX(false), hazard_from_MEM(false) {}
};


class Pipeline {
public:
    PipelineStage stages[5]; 

    void shiftStages() {
        for (int i = 4; i > 0; i--) {
            stages[i] = stages[i - 1];
        }
        stages[0].valid = false; 
        std::cout << "[Pipeline] Shifted stages\n";
    }
};
struct HazardInfo {
    bool hazard_detected;
    bool from_EX;
    bool from_MEM;
    int dependency_reg;
} hazard_info;

class Core {
public:
    const int core_id;
    int pc=0;
    int registers[REGISTER_COUNT]={0};
    std::unordered_map<int, int> visited_pcs;
    std::unordered_map<std::string, int> &label_map; 
    int base_memory_adress;
    Pipeline pipeline;
    int clock_cycles = 0; 
    int stalls = 0;    
    bool enable_forwarding;  
    int latest_ex_result ;
    int latest_ex_rd;
    int hazard_in_id = 0;
    int hazard_in_ex= 0;
    bool stall_flag = false;
    bool hazard_in_sw = false;
    bool hazard_in_lw = false;
    int new_rs1_value_sw ;
    int new_rd_value_sw;

    Core(int id, std::unordered_map<std::string, int> &labels, bool forwarding) : core_id(id), label_map(labels), enable_forwarding(forwarding){
        registers[0]=0;
        registers[31]=core_id;
    } 

    int getRegisterIndex(const std::string &reg) {
        if (reg[0] == 'x') {
            int num = std::stoi(reg.substr(1));
            if (num >= 0 && num < REGISTER_COUNT) return num;
        }
        throw std::invalid_argument("Invalid register: " + reg);
    }

    void fetchInstruction(const std::vector<std::pair<std::string, std::vector<std::string>>> &instructions) {
        if (pc < instructions.size()) {
            pipeline.stages[0].instruction = instructions[pc].first;
            pipeline.stages[0].args = instructions[pc].second;
            pipeline.stages[0].valid = true;
            std::cout << "[Core " << core_id << "] Fetched instruction: " << pipeline.stages[0].instruction << "\n";
            pc++;
        } else {
            pipeline.stages[0].valid = false;
            std::cout << "[Core " << core_id << "] No more instructions to fetch\n";
        }
    }

    void decodeInstruction() {
        hazard_in_id=0;
        if (pipeline.stages[1].valid) {
            auto &instr = pipeline.stages[1].instruction;
            auto &args = pipeline.stages[1].args;
            if (instr == "add" || instr == "sub" || instr == "and" || instr == "or" || instr == "xor") {
                pipeline.stages[1].rd = getRegisterIndex(args[0]);  
                pipeline.stages[1].rs1 = getRegisterIndex(args[1]); 
                pipeline.stages[1].rs2 = getRegisterIndex(args[2]); 
            } 
            else if (instr == "addi" || instr == "slli" || instr == "srli" || instr == "andi" || instr == "ori" || instr == "xori") {
                pipeline.stages[1].rd = getRegisterIndex(args[0]);  
                pipeline.stages[1].rs1 = getRegisterIndex(args[1]); 
                pipeline.stages[1].rs2 = std::stoi(args[2]);  
            } 

            else if (instr == "lw") {
                pipeline.stages[1].rd = getRegisterIndex(args[0]);  
                std::string baseReg = args[1].substr(args[1].find('(') + 1, args[1].find(')') - args[1].find('(') - 1);
                pipeline.stages[1].rs1 = getRegisterIndex(baseReg); 
                pipeline.stages[1].rs2 = -1; 
            } 

            else if (instr == "sw") {
                pipeline.stages[1].rs1 = getRegisterIndex(args[0]);
                std::string baseReg = args[1].substr(args[1].find('(') + 1, args[1].find(')') - args[1].find('(') - 1);
                pipeline.stages[1].rd = getRegisterIndex(baseReg);                 
                pipeline.stages[1].rs2 = -1; 
            } 

            else if (instr == "beq" || instr == "bne" || instr == "blt" || instr == "bge") {
                pipeline.stages[1].rs1 = getRegisterIndex(args[0]); 
                pipeline.stages[1].rs2 = getRegisterIndex(args[1]); 
                pipeline.stages[1].rd = std::stoi(args[2]);
            }

            else if (instr == "jal" || instr == "j") {
                pipeline.stages[1].rd = getRegisterIndex(args[0]);  
                pipeline.stages[1].rs1 = -1;
                pipeline.stages[1].rs2 = -1;
            }
            std::cout << "[Core " << core_id << "] Decoded instruction: " << instr
                      << " | RD: " << pipeline.stages[1].rd
                      << " | RS1: " << pipeline.stages[1].rs1
                      << " | RS2: " << pipeline.stages[1].rs2 << "\n";
        }
        int ex_rd = pipeline.stages[2].rd;
        int mem_rd = pipeline.stages[3].rd;

        if (ex_rd != 0 && (ex_rd == pipeline.stages[1].rs1 || ex_rd == pipeline.stages[1].rs2)) {
            pipeline.stages[1].hazard_detected = true;
            pipeline.stages[1].hazard_from_EX = true;
            hazard_in_id++;
        }
        if (mem_rd != 0 && (mem_rd ==  pipeline.stages[1].rs1 || mem_rd ==  pipeline.stages[1].rs2)) {
            pipeline.stages[1].hazard_detected = true;
            pipeline.stages[1].hazard_from_MEM = true;
            hazard_in_id++;
        }
         cout<< "hazard in id :" << hazard_in_id<<endl;
    }
    

    void executeStage() {
        if (pipeline.stages[2].valid) {
            auto &instr = pipeline.stages[2].instruction;
            auto &args = pipeline.stages[2].args;
    
            int rs1_val = registers[pipeline.stages[2].rs1];
             int rs2_val = registers[pipeline.stages[2].rs2];

            if(hazard_in_ex!=0 && (instr != "lw" && instr != "sw")){
                cout<<"deteced hazard in ex " <<endl;
                    if (enable_forwarding) { 
                            if (pipeline.stages[2].rs1 == pipeline.stages[4].rd) {
                                rs1_val = pipeline.stages[4].result;
                                std::cout << "[Core " << core_id << "] Forwarding x" << pipeline.stages[2].rs1 
                                << " from MEM stage: " <<pipeline.stages[4].result<< "\n";
  
                            }
                            if (pipeline.stages[2].rs2 == pipeline.stages[4].rd) {
                                rs2_val = pipeline.stages[4].result;
                                std::cout << "[Core " << core_id << "] Forwarding x" << pipeline.stages[2].rs2 
                                << " from MEM stage: " << pipeline.stages[4].result<< "\n";
  
                            }
                            if (pipeline.stages[2].rs1 == pipeline.stages[3].rd) {
                                rs1_val = pipeline.stages[3].result;
                                std::cout << "[Core " << core_id << "] Forwarding x" << pipeline.stages[3].rd 
                                      << " from EX computed register: " << pipeline.stages[3].result << "\n";
        
                            }
                            if (pipeline.stages[2].rs2 == pipeline.stages[3].rd) {
                                rs2_val = pipeline.stages[3].result;
                                std::cout << "[Core " << core_id << "] Forwarding x" << pipeline.stages[3].rd 
                                      << " from EX computed stage: " << pipeline.stages[3].result << "\n";
                               }
                     hazard_in_ex =0;
                       cout<<"reset hazard_in_ex:"<<hazard_in_ex<<endl;
                    }
                    else {  
                        cout<<"inserting stall"<<endl;
                        handleStalls();
                        hazard_in_ex =0;
                        cout<<"reset hazard_in_ex"<<hazard_in_ex<<endl;
                     }
            
            }      
            std::cout << "[Core " << core_id << "] Executing instruction: " << instr << "\n";
            if (instr == "add") {
                pipeline.stages[2].result = rs1_val + rs2_val;
            } else if (instr == "sub") {
                pipeline.stages[2].result = rs1_val - rs2_val;
            } else if (instr == "addi") {
                int imm = std::stoi(args[2]);
                pipeline.stages[2].result = rs1_val + imm;
            } else if (instr == "slli") {
                pipeline.stages[2].result = rs1_val << std::stoi(args[2]);
            } else if (instr == "lw") {
                int baseRegIndex = getRegisterIndex(args[1].substr(args[1].find('(') + 1, args[1].find(')') - args[1].find('(') - 1));
                int offset = std::stoi(args[1].substr(0, args[1].find('(')));
                pipeline.stages[2].rs1 = baseRegIndex;  
                cout<< "baseRegIndex: " << baseRegIndex << " offset: " << offset << endl;
                pipeline.stages[2].result = registers[baseRegIndex] + offset;  // Compute memory address
                pipeline.stages[2].rd = getRegisterIndex(args[0]); // Store source register index
            } else if (instr == "sw") {
                int baseRegIndex = getRegisterIndex(args[1].substr(args[1].find('(') + 1, args[1].find(')') - args[1].find('(') - 1));
                int offset = std::stoi(args[1].substr(0, args[1].find('(')));
            
                pipeline.stages[2].rd = baseRegIndex;  
                cout<< "baseRegIndex: " << baseRegIndex << " offset: " << offset << endl;
                pipeline.stages[2].result = registers[baseRegIndex] + offset;  // Compute memory address
                pipeline.stages[2].rs1 = getRegisterIndex(args[0]); // Store source register index

                if(hazard_in_ex==1){
                    cout<<"deteced hazard in ex " <<endl;
                        if (enable_forwarding) { 
                                if (pipeline.stages[2].rd == pipeline.stages[4].rd) {
                                    pipeline.stages[2].result = pipeline.stages[4].result+ offset;
                                    std::cout << "[Core " << core_id << "] Forwarding x" << pipeline.stages[2].rd 
                                    << " from MEM stage: " <<pipeline.stages[4].result<< "\n";
      
                                }
                                if (pipeline.stages[2].rs2 == pipeline.stages[4].rd) {
                                    rs2_val = pipeline.stages[4].result;
                                    std::cout << "[Core " << core_id << "] Forwarding x" << pipeline.stages[2].rs2 
                                    << " from MEM stage: " << pipeline.stages[4].result<< "\n";
      
                                }
                                if (pipeline.stages[2].rs1 == pipeline.stages[3].rd) {
                                    rs1_val = pipeline.stages[3].result;
                                    std::cout << "[Core " << core_id << "] Forwarding x" << pipeline.stages[3].rd 
                                          << " from EX computed register: " << pipeline.stages[3].result << "\n";
            
                                }
                                if (pipeline.stages[2].rs2 == pipeline.stages[3].rd) {
                                    rs2_val = pipeline.stages[3].result;
                                    std::cout << "[Core " << core_id << "] Forwarding x" << pipeline.stages[3].rd 
                                          << " from EX computed stage: " << pipeline.stages[3].result << "\n";
                                   }
                         hazard_in_ex =0;
                           cout<<"reset hazard_in_ex:"<<hazard_in_ex<<endl;
                        }
                        else {  
                            cout<<"inserting stall"<<endl;
                            handleStalls();
                            hazard_in_ex =0;
                            cout<<"reset hazard_in_ex"<<hazard_in_ex<<endl;
                         }
                
                }            
                std::cout << "[Core " << core_id << "] SW Computed Address: " 
                          << pipeline.stages[2].result << " Storing Value: " 
                          << rs1_val << "\n";
                          new_rs1_value_sw = rs1_val;
                          new_rd_value_sw = pipeline.stages[2].result;
            }
            
            
             else if (instr == "beq") {
                if (rs1_val == rs2_val) {
                    pc = label_map[args[2]];
                }
            } else if (instr == "bne") {
                if (rs1_val != rs2_val) {
                    pc = label_map[args[2]];
                }
            } else if (instr == "blt") {
                if (rs1_val < rs2_val) {
                    pc = label_map[args[2]];
                }
            } else if (instr == "j") {
                pc = label_map[args[0]];
            } else if (instr == "jal") {
                registers[getRegisterIndex(args[0])] = pc;
                pc = label_map[args[1]];
            } else if (instr == "li") {
                pipeline.stages[2].result = std::stoi(args[1]);
            } else if (instr == "la") {
                pipeline.stages[2].result = label_map[args[1]];
            } else if (instr == "ecall") {
                if (registers[17] == 10) {
                    std::cout << "[Core " << core_id << "] Exit syscall detected. Terminating program.\n";
                    exit(0);
                }
            }
        latest_ex_result = pipeline.stages[2].result;
        cout << "Latest_ex_result:"<<latest_ex_result<<endl;
        latest_ex_rd = pipeline.stages[2].rd;
           if(hazard_in_id!=0){
            hazard_in_ex++;
            cout << "Since hazard in id, incrementing hazard_in_ex: "<<hazard_in_ex<<endl;
           }
        }
    }

    void memoryStage() {
        std::cout << "[Core " << core_id << "] Memory stage for instruction: " << pipeline.stages[3].instruction << "\n";
        if (pipeline.stages[3].valid) {
            auto &instr = pipeline.stages[3].instruction;
            auto &args = pipeline.stages[3].args;

            if (instr == "lw") {
                cout<< "rs1_val: " << pipeline.stages[3].result << " offset: " << args[1] << endl;
                pipeline.stages[3].result = lw(pipeline.stages[3].result);
            } else if (instr == "sw") {
                sw(new_rd_value_sw, new_rs1_value_sw);
            }

            
        }
    }

    void writeBack() {
        if (pipeline.stages[4].valid) {
            auto &instr = pipeline.stages[4].instruction;
            auto &args = pipeline.stages[4].args;

            if (instr == "add" || instr == "sub" || instr == "addi" || instr == "slli" || instr == "lw" || instr == "li" || instr == "la") {
                registers[getRegisterIndex(args[0])] = pipeline.stages[4].result;
            }

            std::cout << "[Core " << core_id << "] Write back for instruction: " << instr << "\n";
        }
    }

    void printRegisters() {
        std::cout << "Core " << core_id << " Registers:\n";
        for (int i = 0; i < REGISTER_COUNT; i++) {
            std::cout << "x" << i << ": " << registers[i] << "\t";
            if ((i + 1) % 8 == 0) {
                std::cout << std::endl;
            }
        }
    }

   
  void handleStalls() {
            stalls++;
            clock_cycles++;
            stall_flag = true;
            std::cout << "[Core " << core_id << "] Stalling due to hazard... Total Stalls: " << stalls << "\n";
  }

    
    
    
    
    int forwardData(int registerIndex) {
            if (registerIndex == 0) return 0;  // x0 is always zero
        
            if (enable_forwarding) {
                // ✅ 1️⃣ Prefer EX Stage (Most Recent Computation)
                if (latest_ex_rd == registerIndex) {
                    std::cout << "[Core " << core_id << "] Forwarding x" << registerIndex 
                              << " from Stored EX Result: " << latest_ex_result << "\n";
                    return latest_ex_result;
                }
        
                // ✅ 2️⃣ If EX is empty, check MEM Stage (Next Most Recent)
                if (pipeline.stages[3].valid && pipeline.stages[3].rd == registerIndex) {
                    std::cout << "[Core " << core_id << "] Forwarding x" << registerIndex 
                              << " from MEM stage: " << pipeline.stages[3].result << "\n";
                    return pipeline.stages[3].result;
                }
            }
        
            return registers[registerIndex];  // Default register value if no forwarding
        }
        

};    
