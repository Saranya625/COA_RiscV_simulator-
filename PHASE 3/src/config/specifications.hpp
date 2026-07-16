#pragma once
#include <string>
#include <unordered_map>

struct Specifications {
    bool data_forwarding;
    std::string replacement_policy;
    int line_size;
    int L1_cache_size;
    int L2_cache_size;
    int L1_associativity;
    int L2_associativity;
    int L1_latency;
    int L2_latency;
    int main_memory_latency;
    std::unordered_map<std::string, int> latencies;
};

Specifications parseSpecifications(const std::string &filename);
