#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "core.cpp"

std::unordered_map<std::string, int> label_map;  // Labels and their addresses
std::vector<std::pair<std::string, std::vector<std::string>>> instructions;  // Instructions

// Parse assembly file
void parseAssembly(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }

    std::string line;
    int index = 0;
    while (std::getline(file, line)) {
        size_t commentPos = line.find("#");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string first, instr;
        iss >> first;

        if (first.back() == ':') {
            first.pop_back();
            label_map[first] = index;
        } else {
            instr = first;
            std::vector<std::string> args;
            std::string arg;
            while (std::getline(iss, arg, ',')) {
                while (!arg.empty() && (arg[0] == ' ' || arg[0] == '\t')) arg.erase(0, 1);
                args.push_back(arg);
            }
            instructions.emplace_back(instr, args);
            index++;
        }
    }
}

// Execute instructions
void executeProgram(Core &core) {
    while (core.pc < instructions.size()) {
        auto &instruction = instructions[core.pc];
        const std::string &instr = instruction.first;
        const std::vector<std::string> &args = instruction.second;
        if (core.visited_pcs.find(core.pc) != core.visited_pcs.end()) {
            std::cout << " Infinite loop detected at PC " << core.pc << " on core " << core.core_id << "\n";
            break;
        }
        core.visited_pcs.insert(core.pc);

        try {
            if (instr == "add") {
                core.registers[core.getRegisterIndex(args[0])] =
                    core.registers[core.getRegisterIndex(args[1])] + core.registers[core.getRegisterIndex(args[2])];
            } else if (instr == "sub") {
                core.registers[core.getRegisterIndex(args[0])] =
                    core.registers[core.getRegisterIndex(args[1])] - core.registers[core.getRegisterIndex(args[2])];
            } else if (instr == "bne") {
                if (core.registers[core.getRegisterIndex(args[0])] != core.registers[core.getRegisterIndex(args[1])]) {
                    core.pc = label_map[args[2]];
                    continue;
                }
            } else if (instr == "jal") {
                core.pc = label_map[args[1]];
                continue;
            } else if (instr == "lw") {
                core.registers[core.getRegisterIndex(args[0])] = lw(std::stoi(args[1]));
            } else if (instr == "sw") {
                sw(std::stoi(args[1]), core.registers[core.getRegisterIndex(args[0])]);
            } else if (instr == "addi") {
                core.registers[core.getRegisterIndex(args[0])] =
                    core.registers[core.getRegisterIndex(args[1])] + std::stoi(args[2]);
            } else {
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
    for (int i = 0; i < 4; i++) cores.emplace_back(i);

    parseAssembly("assembly.txt");
    for (Core &core : cores) executeProgram(core);

    // Print registers and memory
    for (Core &core : cores) core.printRegisters();
    printMemory();

    return 0;
}
