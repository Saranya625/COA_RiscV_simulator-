#pragma once
#include <cstdint>
#include <vector>

class SP_memory {
public:
    int size_sp;
    int latency_sp;
    int associativity_sp;
    std::vector<uint8_t> sp_memory;

    SP_memory(int size, int latency, int associativity);

    int lw_spm(int address);
    void sw_spm(int address, int value);
    void sp_printMemory();
    int readWord(int address) const;
};
