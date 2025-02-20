#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <thread>
#include "core.cpp"
using namespace std;

const int CORE_COUNT = 4;
const int CORE_MEMORY_SIZE = 1024; // 1KB per core
const int TOTAL_MEMORY_SIZE = CORE_COUNT * CORE_MEMORY_SIZE;

int memory[TOTAL_MEMORY_SIZE] = {0};
std::unordered_map<std::string, int> label_map;  
std::vector<std::pair<std::string, std::vector<std::string>>> instructions;  

struct CoreMemory {
    int base_address;
    int size;
};

std::vector<CoreMemory> core_memory_allocations(CORE_COUNT);


void parseAssembly(const std::string &filename , int core_id) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }


    std::string line;
    bool inTextSection = false;
    bool inDataSection = false;
    int memoryAddress ;
    if(core_id == 0){
        memoryAddress = 0;
    }else if(core_id == 1){
        memoryAddress = 1024;
    }else if(core_id == 2){
      memoryAddress = 2048;}
    else{
         memoryAddress = 3072;
     }


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
            std::cout << "Entering Data Section" << std::endl;
            continue;
        } else if (first == ".text") {
            inTextSection = true;
            inDataSection = false;
            std::cout << "Entering Text Section" << std::endl;
            continue;
        }
        if (inDataSection) {
            if (first.back() == ':') {  
                first.pop_back();  
                label_map[first] = memoryAddress;  
                if (!(iss >> instr)) continue;
            } else {
                instr = first;  
            }
       
            std::vector<std::string> args;
            std::string arg;
           
            cout << "Instruction: " << instr << endl;
            if (iss.peek() != EOF) {
                while (std::getline(iss, arg, ',')) {
                    arg.erase(0, arg.find_first_not_of(" \t"));  
                    arg.erase(arg.find_last_not_of(" \t") + 1);
                    args.push_back(arg);
                    cout << "Parsed argument: " << arg << endl;
                }
            }
       
            if (instr == ".word") {
                std::cout << "Entering word Section" << std::endl;
                std::string value;
                while (iss >> value) {
                    args.push_back(value);
                }
                if (!args.empty()) {
                    cout << "args size: " << args.size() << endl;
                    label_map[first] = memoryAddress;
                    std::cout << "Allocated memory for " << first << " at 0x"
                              << std::hex << memoryAddress << std::endl;
           
                    for (size_t i = 0; i < args.size(); i++) {
                        int value = std::stoi(args[i]);
                        cout<< "adress: " <<std::hex << memoryAddress + (i * 4) << " value: " << value << endl;
                        sw(memoryAddress + (i * 4), value); // Store values in memory
                    }
                } else {
                    std::cout << "Error: No values found for .word directive!" << std::endl;
                }
            }
        }
       
        else if (inTextSection) {
            if (first.back() == ':') {
                first.pop_back();  
                label_map[first] = instructions.size();
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
               
                instructions.emplace_back(instr, args);
            }
        }
    }
}


void executeProgram(Core &core) {
    while (core.pc < instructions.size()) {
        auto &instruction = instructions[core.pc];
        const std::string &instr = instruction.first;
        const std::vector<std::string> &args = instruction.second;
        core.visited_pcs[core.pc]++;  
        if (core.visited_pcs[core.pc] > 1000) {  
            std::cout << "Infinite loop detected at PC " << core.pc << " on core " << core.core_id << "\n";
            break;
        }
 


        try {
             cout << "executed instruction "<< instr << endl;  
            if (instr == "add") {
                core.registers[core.getRegisterIndex(args[0])] =
                    core.registers[core.getRegisterIndex(args[1])] + core.registers[core.getRegisterIndex(args[2])];
            }
            else if (instr == "sub") {
                core.registers[core.getRegisterIndex(args[0])] =
                    core.registers[core.getRegisterIndex(args[1])] - core.registers[core.getRegisterIndex(args[2])];
            }
            else if (instr == "bne") {
                int rs1 = core.getRegisterIndex(args[0]);
                int rs2 = core.getRegisterIndex(args[1]);
           
                std::cout << "bne Debug: x" << rs1 << " = " << core.registers[rs1]
                          << ", x" << rs2 << " = " << core.registers[rs2] << std::endl;
           
                if (core.registers[rs1] != core.registers[rs2]) {
                    if (label_map.find(args[2]) != label_map.end()) {
                        std::cout << "Branching to " << args[2] << " at PC = " << label_map[args[2]] << std::endl;
                        core.pc = label_map[args[2]];  
                        continue;  
                    } else {
                        std::cerr << "Error: Label not found: " << args[2] << std::endl;
                        exit(1);
                    }
                }
            }
       
             else if (instr == "jal") {
                core.pc = label_map[args[1]];
                continue;
            }
            else if (instr == "j") {
                if (label_map.find(args[0]) != label_map.end()) {
                    core.pc = label_map[args[0]];  
                    continue;
                } else {
                    std::cerr << "Error: Label not found: " << args[0] << std::endl;
                    exit(1);
                }
            }
           
            else if (instr == "lw") {
                int rd = core.getRegisterIndex(args[0]);  
                int offset, rs1;
                size_t openParen = args[1].find('(');
                size_t closeParen = args[1].find(')');
           
                if (openParen == std::string::npos || closeParen == std::string::npos) {
                    throw std::invalid_argument("Invalid memory access syntax in lw instruction");
                }
                offset = std::stoi(args[1].substr(0, openParen));  
                rs1 = core.getRegisterIndex(args[1].substr(openParen + 1, closeParen - openParen - 1));  
                int address = core.registers[rs1] + offset;  
                core.registers[rd] = lw(address);  
                cout<<"adress"<< address << " value: " << core.registers[rd] << endl;
            }
           
            else if (instr == "sw") {
                std::cout << "Parsing SW instruction: " << args[0] << ", " << args[1] << std::endl;
                int rs = core.getRegisterIndex(args[0]);  
                int offset, rd;
                cout << "rs: " << rs << endl;
                size_t openParen = args[1].find('(');
                size_t closeParen = args[1].find(')');
               
                if (openParen == std::string::npos || closeParen == std::string::npos) {
                    throw std::invalid_argument("Invalid memory access syntax in sw instruction");
                }
           
                offset = std::stoi(args[1].substr(0, openParen));
                rd = core.getRegisterIndex(args[1].substr(openParen + 1, closeParen - openParen - 1));
                cout << "offset: " << offset << " rd: " << rd << endl;
           
                int address = core.registers[rd] + offset;
           
                sw(address, core.registers[rs]);
            }
           
            else if (instr == "addi") {
                int rd = core.getRegisterIndex(args[0]);  
                cout<< "rd: " << rd << endl;
                int rs1 = core.getRegisterIndex(args[1]);
                cout<< "rs1: " << rs1 << endl;
                int imm = stoi(args[2]);
                cout<< "imm: " << imm << endl;            
               core.registers[rd] = core.registers[rs1] + imm;
            }
            else if (instr == "slli") {
                int rd = core.getRegisterIndex(args[0]);  // Destination register
                int rs1 = core.getRegisterIndex(args[1]); // Source register
                int shamt = stoi(args[2]); // Shift amount (immediate value)
            
                // Debug output
                std::cout << "slli Debug: x" << rd << " = x" << rs1 << " << " << shamt << std::endl;
                std::cout << "Before shift: x" << rs1 << " = " << core.registers[rs1] << std::endl;
            
                // Perform logical shift left
                core.registers[rd] = core.registers[rs1] << shamt;
            
                // Debug output after shift
                std::cout << "After shift: x" << rd << " = " << core.registers[rd] << std::endl;
            }
            else if (instr == "blt") {
                int rs1 = core.getRegisterIndex(args[0]);
                int rs2 = core.getRegisterIndex(args[1]);
           
                std::cout << "blt Debug: x" << rs1 << " = " << core.registers[rs1]
                          << ", x" << rs2 << " = " << core.registers[rs2] << std::endl;
           
                if (core.registers[rs1] <= core.registers[rs2]) {
                    if (label_map.find(args[2]) != label_map.end()) {
                        std::cout << "Branching to " << args[2] << " at PC = " << label_map[args[2]] << std::endl;
                        core.pc = label_map[args[2]];  
                        continue;  
                    } else {
                        std::cerr << "Error: Label not found: " << args[2] << std::endl;
                        exit(1);
                    }
                }
            }
            
            
            else if (instr == "la") {
                if (label_map.find(args[1]) != label_map.end()) {
                    int address = label_map[args[1]];
                    core.registers[core.getRegisterIndex(args[0])] = address;
                    std::cout << "Loaded address of " << args[1] << " (" << address << ") into " << args[0] << std::endl;
                } else {
                    std::cerr << "Error: Label not found: " << args[1] << std::endl;
                    exit(1);
                }
            }
            else if (instr == "beq") {
                int rs1 = core.getRegisterIndex(args[0]);
                int rs2 = core.getRegisterIndex(args[1]);
           
                cout << "Checking beq: rs1=" << core.registers[rs1]
                     << ", rs2=" << core.registers[rs2] << endl;
           
                if (core.registers[rs1] == core.registers[rs2]) {
                    if (label_map.find(args[2]) != label_map.end()) {
                        core.pc = label_map[args[2]];  
                        continue;
                    } else {
                        cerr << "Error: Label not found: " << args[2] << endl;
                        exit(1);
                    }
                }
            }
           
            else if (instr == "li") {
                int rd = core.getRegisterIndex(args[0]);
                int imm = std::stoi(args[1]);
                core.registers[rd] = imm;
            }
            else if (instr == "ecall") {
                if (core.registers[17] == 10) {  
                    std::cout << "Exit syscall detected. Terminating program." << std::endl;
                    break;
                }
            }
            else if ( instr == "nop") {
               break;
            }
             
            else {
                std::cout << "Unknown instruction: " << instr << std::endl;
            }
        } catch (std::exception &e) {
            std::cout << "Error processing instruction: " << instr << " -> " << e.what() << std::endl;
        }
        core.pc++;
    }
}


int main() {
    std::vector<Core> cores;
    cout << "Simulator started!" << endl;
    for (int i = 0; i < 4; i++) cores.emplace_back(i);
    cout<< "intial" << endl;
    cout << "Reading assembly file..." << endl;
    for(Core &core : cores){
      parseAssembly("assembly.asm",core.core_id); 
        executeProgram(core);
        core.printRegisters();
    }
    cout<< "final" << endl;
   printMemory();

    return 0;
}
