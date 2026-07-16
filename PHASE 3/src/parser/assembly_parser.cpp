#include "assembly_parser.hpp"
#include "../common/constants.hpp"
#include "../memory/memory.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

using namespace std;

unordered_map<string, vector<int>> label_map;

static string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

void parseAssembly(const std::string &filename) { 
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }

    std::string line;
    bool inTextSection = false, inDataSection = false;
    while (std::getline(file, line)) {
        size_t commentPos = line.find("#");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line = trim(line);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string first, instr;
        if (!(iss >> first)) continue;

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
            string dataLabel;
            if (!first.empty() && first.back() == ':') {
                first.pop_back();
                label_map[first] = std::vector<int>(CORE_COUNT, 0);
                dataLabel = first;
                if (!(iss >> instr)) continue;
            } else {
                instr = first;
            }

            std::vector<std::string> args;
            std::string arg;
            while (std::getline(iss, arg, ',')) {
                arg = trim(arg);
                if (!arg.empty()) args.push_back(arg);
            }

            if (instr == ".word") {
                if (dataLabel.empty()) {
                    std::cerr << "Error: .word must follow a data label in line: " << line << std::endl;
                    exit(1);
                }
                int size = args.size();
                for (int i = 0; i < CORE_COUNT; i++) {
                    int memoryAddress = allocate_memory(size, i);
                    label_map[dataLabel][i] = memoryAddress;  // Store per-core base address
                    for (size_t j = 0; j < args.size(); j++) {
                        int value = std::stoi(args[j]);
                        sw1(memoryAddress + (j * 4), value, i);
                    }
                }
            }
        } else if (inTextSection) {
            if (!first.empty() && first.back() == ':') {
                first.pop_back();
                label_map[first] = std::vector<int>(CORE_COUNT, instruction_size());
                if (!(iss >> instr)) continue;
            } else {
                instr = first;
            }

            std::vector<std::string> args;
            std::string restOfLine;
            std::getline(iss, restOfLine);
            std::istringstream argStream(restOfLine);
            std::string token;
            while (std::getline(argStream, token, ',')) {
                token = trim(token);
                if (!token.empty()) args.push_back(token);
            }
            store_instruction(instr, args);
        }
    }
    
}
