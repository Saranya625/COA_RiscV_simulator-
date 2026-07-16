#pragma once
#include <string>
#include <unordered_map>
#include <vector>

// Populated by parseAssembly(): maps each label to a per-core address
// (data labels -> allocated memory address, text labels -> instruction PC).
extern std::unordered_map<std::string, std::vector<int>> label_map;

void parseAssembly(const std::string &filename);
