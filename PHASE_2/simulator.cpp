#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "core.cpp"

std::vector<std::pair<std::string, std::vector<std::string>>> instructions;
std::unordered_map<std::string, int> label_map;

void parseAssembly(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }

    std::string line;
    bool inTextSection = false;
    bool inDataSection = false;
    int memoryAddress = 0;

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
                label_map[first] = memoryAddress;
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
                memoryAddress = allocate_memory(size * 4); // Allocate memory for .word
                for (size_t i = 0; i < args.size(); i++) {
                    int value = std::stoi(args[i]);
                    sw(memoryAddress + (i * 4), value);
                }
            }
        } else if (inTextSection) {
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

void executePipeline(Core &core) {
    bool running = true;

    while (running) {
        std::cout << "\n====== [Clock Cycle: " << core.clock_cycles+1 << "] ======" << std::endl;
        
        core.writeBack();
        core.memoryStage();
        core.executeStage();
        core.decodeInstruction();
        core.fetchInstruction(instructions);
        core.pipeline.shiftStages();
       
        core.clock_cycles++;
         running=false;
        running =  (core.pipeline.IF_ID.valid_instruction || core.pipeline.ID_EX.valid_instruction ||  core.pipeline.EX_MEM.valid_instruction || core.pipeline.MEM_WB.valid_instruction || core.pipeline.WB_Return.valid_instruction); 
                 
                   
    
}
}
 


int main() {
    std::vector<Core> cores;
    bool forwarding_option;
   std::cout << "Enable data forwarding? (1 = Yes, 0 = No): ";
   std::cin >> forwarding_option;
    for (int i = 0; i < 1; i++) cores.emplace_back(i,label_map,forwarding_option);

    parseAssembly("assembly.asm");

    for (Core &core : cores) {
        executePipeline(core);
        core.printRegisters();
        cout << "Clock Cycles: " << core.clock_cycles << endl;
        cout<<"Stalls: "<<core.pipeline.stalls<<endl;
        cout << endl;
    }

    printMemory();

    return 0;
}