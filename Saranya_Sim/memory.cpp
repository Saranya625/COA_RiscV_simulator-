#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#define MEMORY_SIZE 4096  // 4KB shared memory
#define REGISTER_COUNT 32  // 32 registers
uint8_t memory[MEMORY_SIZE] = {0}; 
int registers[REGISTER_COUNT] = {0};
void assignRandomAddresses() {
    std::srand(std::time(nullptr)); 

    for (int i = 0; i < REGISTER_COUNT; i++) {
        registers[i] = (std::rand() % (MEMORY_SIZE / 4)) * 4;
    }
}
void printRegisters_memory() {
    std::cout << "\n=== Register State ===\n";
    for (int i = 0; i < REGISTER_COUNT; i++) {
        std::cout << "x" << i << " = " << registers[i] << "\t";
        if ((i + 1) % 8 == 0) std::cout << std::endl;  // Print 8 per line
    }
    std::cout << std::endl;
}

int lw(int address) {
    if (address < 0 || address >= MEMORY_SIZE - 3) {
        throw std::out_of_range("Memory access out of bounds");
    }
    return *(int*)(memory + address);
}

void sw(int address, int value) {
    if (address < 0 || address >= MEMORY_SIZE - 3) {
        throw std::out_of_range("Memory access out of bounds");
    }
    *(int*)(memory + address) = value;
}

void printMemory() {
    std::cout << "\n=== Memory State ===\n";
    for (int addr = 0; addr < MEMORY_SIZE; addr += 4) {
        int value = lw(addr);
        std::cout << "0x" << std::hex << addr << ": " << std::dec << value << "\t";
        if ((addr / 4 + 1) % 8 == 0) std::cout << std::endl;  // Print 8 values per line
    }
    std::cout << std::endl;
}


