#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "core.cpp"
using namespace std;

vector<std::pair<string, vector<string>>> instructions;
unordered_map<string, int> label_map;
unordered_map<string , int >latencies;
int total_instructions=0;


void parseLabels(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }

    std::string line;
    bool inTextSection = false;
    int instructionIndex = 0; // Track instruction positions for labels

    while (std::getline(file, line)) {
        size_t commentPos = line.find("#");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        if (line.empty()) continue;  // Skip empty lines

        std::istringstream iss(line);
        std::string first;
        iss >> first;

        if (first == ".text") {
            inTextSection = true;
            continue;
        }

        if (inTextSection) {
            
            if (first.back() == ':') {  
                first.pop_back(); // Remove ':' from label name
                label_map[first] = instructionIndex; // Store label's position
                std::cout << "Stored label: " << first << " at index " << instructionIndex << std::endl;
            } else {
                instructionIndex++; // Count only actual instructions
            }
        }
    }

    file.close(); // Close the file after first pass
}


void parseAssembly(const std::string &filename,int core_id) {
    std::ifstream file(filename);
    parseLabels(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }

    std::string line;
    bool inTextSection = false;
    bool inDataSection = false;
    int memoryAddress ;


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
        else{
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
    std::cout << "\nParsed Assembly Instructions:\n";
    for (size_t i = 0; i < instructions.size(); i++) {
        std::cout << i+1 << ": " << instructions[i].first;
        total_instructions++;
        for (const auto &arg : instructions[i].second) {
            std::cout << " " << arg;
        }
        std::cout << std::endl;
    }
}

void executePipeline(Core &core) {
    cout << " Pipeline in core:"<<core.core_id << endl;
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
        //running=false;
        running =  (core.pipeline.IF_ID.valid_instruction || core.pipeline.ID_EX.valid_instruction ||  core.pipeline.EX_MEM.valid_instruction || core.pipeline.MEM_WB.valid_instruction || core.pipeline.WB_Return.valid_instruction);     
}

}
 
void getInstructionLatencies() {
    int num_instructions;
    std::cout << "\nEnter the number of arithmetic instructions with variable latencies: ";
    cin >> num_instructions;

    for (int i = 0; i < num_instructions; i++) {
        std::string instr;
        int latency;
        std::cout << "Enter instruction name and latency (e.g.,  1): ";
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
    
    parseAssembly("assembly.asm",0);
    getInstructionLatencies();
    for (int i = 0; i < 4; i++) cores.emplace_back(i,label_map,forwarding_option,latencies);
    for (Core &core : cores) {
 
        executePipeline(core);
        core.printRegisters();
        cout << "Clock Cycles: " << core.clock_cycles << endl;
        cout<<"Stalls: "<<core.pipeline.stalls<<endl;
        cout << "Number of instructions :"<< core.number_instructions << endl;
        double IPC = (double) core.number_instructions / core.clock_cycles;
        cout << " IPC:" << IPC << endl;
        cout << endl;
        core.pipeline.stalls=0;
        core.number_instructions=0;
    }

   printMemory();

    return 0;
}