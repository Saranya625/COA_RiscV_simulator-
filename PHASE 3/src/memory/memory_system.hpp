#pragma once
#include <string>

// Top-level orchestration: wires up the data cache and instruction cache
// from parsed Specifications, and prints combined memory/cache statistics.

void initializeSystem(int line_size1, int l1_size, int l2_size, int l1_assoc,
                       int l2_assoc, int l1_lat, int l2_lat,
                       std::string repl_policy, int main_mem_lat);
void printMemory();
