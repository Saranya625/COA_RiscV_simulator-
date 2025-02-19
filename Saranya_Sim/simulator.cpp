#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "core.cpp"
#include <algorithm>
using namespace std;

std::unordered_map<std::string, int> label_map;  
std::vector<std::pair<std::string, std::vector<std::string>>> instructions;  

void parseAssembly(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }

    std::string line;
    bool inTextSection = false;
    bool inDataSection = false;
    int memoryAddress = 0x0010;  

    while (std::getline(file, line)) {
        // Remove comments
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

        // Handling .data section
        if (inDataSection) {
            if (first.back() == ':') {
                first.pop_back();  // Remove ':'
                label_map[first] = memoryAddress;
            } else {
                instr = first;
                std::vector<std::string> args;
                std::string arg;

                // Read arguments split by commas
                while (std::getline(iss, arg, ',')) {
                    arg.erase(0, arg.find_first_not_of(" \t"));  // Trim leading spaces
                    arg.erase(arg.find_last_not_of(" \t") + 1);  // Trim trailing spaces
                    args.push_back(arg);
                }

                if (instr == ".word") {
                    for (const auto &val : args) {
                        int value = std::stoi(val);
                        sw(memoryAddress, value);  // Store word in memory
                        memoryAddress += 4;  // Move to next word
                    }
                }
            }
        }
        // Handling .text section (instructions)
        else if (inTextSection) {
            if (first.back() == ':') {
                first.pop_back();  // Remove ':'
                label_map[first] = instructions.size();
            } else {
                instr = first;
                std::vector<std::string> args;
                std::string restOfLine;
                std::getline(iss, restOfLine);  // Read the rest of the line

                std::istringstream argStream(restOfLine);
                std::string token;

                // Read arguments split by commas
                while (std::getline(argStream, token, ',')) {
                    token.erase(0, token.find_first_not_of(" \t"));  // Trim leading spaces
                    token.erase(token.find_last_not_of(" \t") + 1);  // Trim trailing spaces
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
        if (core.visited_pcs[core.pc] > 1000) {  // Change 1000 to a reasonable value
            std::cout << "Infinite loop detected at PC " << core.pc << " on core " << core.core_id << "\n";
            break;
        }
        //core.visited_pcs.insert(core.pc);

        try {
             cout << "executed instruction "<< instr << endl;   
            if (instr == "add") {
                core.registers[core.getRegisterIndex(args[0])] =
                    core.registers[core.getRegisterIndex(args[1])] + core.registers[core.getRegisterIndex(args[2])];
            } else if (instr == "sub") {
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
                        core.pc = label_map[args[2]];  // Jump to correct label
                        continue;  // Prevent pc++ after branching
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
                    core.pc = label_map[args[0]];  // Jump to label
                    continue;
                } else {
                    std::cerr << "Error: Label not found: " << args[0] << std::endl;
                    exit(1);
                }
            }
            
            else if (instr == "lw") {
                int rd = core.getRegisterIndex(args[0]);  // Destination register
                int offset, rs1;
            
                // Parse offset(rs1) format
                size_t openParen = args[1].find('(');
                size_t closeParen = args[1].find(')');
            
                if (openParen == std::string::npos || closeParen == std::string::npos) {
                    throw std::invalid_argument("Invalid memory access syntax in lw instruction");
                }
            
                offset = std::stoi(args[1].substr(0, openParen));  // Extract offset
                rs1 = core.getRegisterIndex(args[1].substr(openParen + 1, closeParen - openParen - 1));  // Extract rs1
            
                int address = core.registers[rs1] + offset;  // Compute memory address
            
                core.registers[rd] = lw(address);  // Load word into destination register
            }
            
            else if (instr == "sw") {
                std::cout << "Parsing SW instruction: " << args[0] << ", " << args[1] << std::endl;
                int rd = core.getRegisterIndex(args[0]);   // Destination register
                int offset, rs1;
                cout << "rd: " << rd << endl;
                size_t openParen = args[1].find('(');
                size_t closeParen = args[1].find(')');
                
                if (openParen == std::string::npos || closeParen == std::string::npos) {
                    throw std::invalid_argument("Invalid memory access syntax in sw instruction");
                }
            
                offset = std::stoi(args[1].substr(0, openParen));
                rs1 = core.getRegisterIndex(args[1].substr(openParen + 1, closeParen - openParen - 1));
                cout << "offset: " << offset << " rs1: " << rs1 << endl;
            
                int address = core.registers[rs1] + offset;
            
                sw(address, core.registers[rd]);
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
                        core.pc = label_map[args[2]];  // Jump to label if rs1 == rs2
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
                if (core.registers[17] == 10) {  // a7 = 10 for exit syscall
                    std::cout << "Exit syscall detected. Terminating program." << std::endl;
                    break;
                }
            }
            else if ( instr == "nop") {
                
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
    //printRegisters_memory();
    parseAssembly("assembly.asm");
    cout << "Reading assembly file..." << endl;
    for (Core &core : cores) executeProgram(core);
    for (Core &core : cores) core.printRegisters();
    cout<< "final" << endl;
    printRegisters_memory();
    printMemory();

    return 0;
}
