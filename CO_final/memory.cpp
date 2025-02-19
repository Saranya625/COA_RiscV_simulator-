#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#define MEMORY_SIZE 4096  
#define REGISTER_COUNT 32  
uint8_t memory[MEMORY_SIZE] = {0}; 
int registers[REGISTER_COUNT] = {0};
std::unordered_map<uint32_t, uint32_t> memory_map;
uint32_t heap_pointer = 0x010;  
uint32_t allocate_memory(size_t num_elements) {
    int allocated_address = heap_pointer;
    heap_pointer += num_elements * 4;  // Allocate `size` words (4 bytes each)
    return allocated_address;
}
void printRegisters_memory() {
    std::cout << "\n=== Register State ===\n";
    for (int i = 0; i < REGISTER_COUNT; i++) {
        std::cout << "x" << i << " = " << registers[i] << "\t";
        if ((i + 1) % 8 == 0) std::cout << std::endl; 
    }
    std::cout << std::endl;
}

int lw(int address) {
    if (address < 0 || address >= MEMORY_SIZE - 3) {
        throw std::out_of_range("Memory access out of bounds");
    }
    int value = *(int*)(memory + address);
    return value;
}
void sw(int address, int value) {
    if (address < 0 || address >= MEMORY_SIZE - 3) {
        throw std::out_of_range("Memory access out of bounds");
    }
    *(int*)(memory + address) = value;  // Ensure proper storage
    std::cout << "Stored " << value << " at 0x" << std::hex << address << std::endl;
}


void printMemory() {
    std::cout << "\n=== Memory State ===\n";
    for (int addr = 0; addr < MEMORY_SIZE; addr += 4) {
        int value = lw(addr);
        std::cout << "0x" << std::hex << addr << ": " << std::dec << value << "\t";
        if ((addr / 4 + 1) % 8 == 0) std::cout << std::endl;  
    }
    std::cout << std::endl;
}


