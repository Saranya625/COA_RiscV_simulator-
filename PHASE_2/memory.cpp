#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unordered_map>
using namespace std;
#define MEMORY_SIZE 4096  
#define REGISTER_COUNT 32  

uint8_t memory_main[MEMORY_SIZE] = {0}; 
int registers[REGISTER_COUNT] = {0};
std::unordered_map<uint32_t, uint32_t> memory_map;
uint32_t heap_pointer = 0x010;  
int base_adress;

uint32_t allocate_memory(size_t num_elements) {
    int allocated_address = heap_pointer;
    heap_pointer += num_elements * 4;
    return allocated_address;
}

int lw(int address) {
    if (address < 0 || address > MEMORY_SIZE - sizeof(int)) {
        throw std::out_of_range("Memory access out of bounds");
    }
    int value;
    std::memcpy(&value, &memory_main[address]-base_adress, sizeof(int));
    
    
    return value;
}


void sw(int address, int value) {
    if (address < 0 || address >= MEMORY_SIZE - sizeof(int)) {
        throw std::out_of_range("Memory access out of bounds");
    }
    int old_value = lw(address+base_adress);
    cout<< "old value: " << old_value << endl;
    std::cout << "Before Store: Memory[0x" << std::hex << address << "] = " << std::dec << old_value << std::endl;
    std::cout << "Writing Value: " << value << " to Address: 0x" << std::hex << address << std::endl;
    std::memcpy(&memory_main[address]+base_adress, &value, sizeof(int));
    int new_value = lw(address+base_adress);
    std::cout << "After Store:  Memory[0x" << std::hex << address << "] = " << std::dec << new_value << std::endl;
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

