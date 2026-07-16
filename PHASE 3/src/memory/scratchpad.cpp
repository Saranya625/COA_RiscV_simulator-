#include "scratchpad.hpp"
#include <cstring>
#include <iostream>
#include <cstdlib>

SP_memory::SP_memory(int size, int latency, int associativity)
    : size_sp(size), latency_sp(latency), associativity_sp(associativity), sp_memory(size, 0) {}

int SP_memory::lw_spm(int address) {
    if (address < 0 || address + 3 >= size_sp) {
        std::cerr << "Error: Scratchpad read out of bounds at address " << address << "\n";
        exit(1);
    }
    int value;
    std::memcpy(&value, &sp_memory[address], sizeof(int));
    return value;
}

void SP_memory::sw_spm(int address, int value) {
    if (address < 0 || address + 3 >= size_sp) {
        std::cerr << "Error: Scratchpad write out of bounds at address " << address << "\n";
        exit(1);
    }
    std::memcpy(&sp_memory[address], &value, sizeof(int));
}

void SP_memory::sp_printMemory() {
    std::cout << "\n=== ScratchPad Memory State ===\n";
    for (int addr = 0; addr < size_sp; addr += 4) {
        int value = sp_memory[addr];
        std::cout << "0x" << std::hex << addr << ": " << std::dec << value << "\t";
        if ((addr / 4 + 1) % 8 == 0) std::cout << std::endl;  
    }
    std::cout << std::endl;
}

int SP_memory::readWord(int address) const {
    int value = 0;
    if (address >= 0 && address + 3 < size_sp) {
        std::memcpy(&value, &sp_memory[address], sizeof(int));
    }
    return value;
}
