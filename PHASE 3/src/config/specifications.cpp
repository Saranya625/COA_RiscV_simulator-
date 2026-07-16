#include "specifications.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdlib>

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
