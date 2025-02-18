#include <iostream>
#include <cstdint>

#define MEMORY_SIZE 4096  // 4KB shared memory

uint8_t memory[MEMORY_SIZE] = {0};  // Shared memory

// Load word from memory
int lw(int address) {
    if (address < 0 || address >= MEMORY_SIZE - 3) {
        throw std::out_of_range("Memory access out of bounds");
    }
    return *(int*)(memory + address);
}

// Store word to memory
void sw(int address, int value) {
    if (address < 0 || address >= MEMORY_SIZE - 3) {
        throw std::out_of_range("Memory access out of bounds");
    }
    *(int*)(memory + address) = value;
}

// Print memory contents
void printMemory() {
    std::cout << "\n=== Memory State ===\n";
    for (int addr = 0; addr < MEMORY_SIZE; addr += 4) {
        std::cout << "0x" << std::hex << addr << ": " << std::dec << *(int*)(memory + addr) << "\t";
        if ((addr / 4 + 1) % 8 == 0) std::cout << std::endl;  // Print 8 values per line
    }
    std::cout << std::endl;
}
