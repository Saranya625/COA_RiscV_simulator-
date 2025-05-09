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
uint32_t heap_pointers[4] = {0, 1024, 2048, 3072};  // Separate pointers for each core
int core_id;
uint32_t allocate_base_adress(int core_id) {
    if (core_id < 0 || core_id >= 4) {
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }
    return heap_pointers[core_id];  // Return the base address for the given core
}


uint32_t allocate_memory(size_t num_elements, int core_id) {
    if (core_id < 0 || core_id >= 4) {
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }

    uint32_t base_address = allocate_base_adress(core_id);
    uint32_t &heap_pointer = heap_pointers[core_id]; // Reference to the core's heap pointer

    if (heap_pointer + (num_elements * 4) >= base_address + 1024) {
        std::cerr << "Error: Core " << core_id << " exceeded its 1KB memory limit!\n";
        exit(1);
    }

    uint32_t allocated_address = heap_pointer;
    heap_pointer += num_elements * 4;
    return allocated_address;
}


int lw(int address) {
    if (address < 0 || address > MEMORY_SIZE - sizeof(int)) {
        throw std::out_of_range("Memory access out of bounds");
    }
    int value;
    std::memcpy(&value, &memory_main[address], sizeof(int));
    
    
    return value;
}
int lw(int address, int core_id) {
    int base_address = core_id * 1024;
    int max_address = base_address + 1024;
    if (address < base_address || address >= MEMORY_SIZE) {
        std::cerr << "Error: Core " << core_id << " tried to access memory out of its range!\n";
        exit(1);
    }

    int value;
    std::memcpy(&value, &memory_main[address], sizeof(int));
    return value;
}


/*void sw(int address, int value) {
    if (address < 0 || address >= MEMORY_SIZE - sizeof(int)) {
        throw std::out_of_range("Memory access out of bounds");
    }
    int old_value = lw(address);
    cout<< "old value: " << old_value << endl;
    std::cout << "Before Store: Memory[0x" << std::hex << address << "] = " << std::dec << old_value << std::endl;
    std::cout << "Writing Value: " << value << " to Address: 0x" << std::hex << address << std::endl;
    std::memcpy(&memory_main[address], &value, sizeof(int));
    int new_value = lw(address);
    std::cout << "After Store:  Memory[0x" << std::hex << address << "] = " << std::dec << new_value << std::endl;
}*/
void sw(int address, int value, int core_id) {
    int base_address = (core_id) * 1024;
    int max_address = base_address + 1024;
    if (address < 0 || address >= MEMORY_SIZE) {
        std::cerr << "Error: Core " << core_id << " tried to access memory out of its range!\n";
        exit(1);
    }

    int old_value = lw(address, core_id);
    std::cout << "Before Store: Memory[0x" << std::hex << address << "] = " << std::dec << old_value << std::endl;
    std::cout << "Writing Value: " << value << " to Address: 0x" << std::hex << address << std::endl;
    std::memcpy(&memory_main[address], &value, sizeof(int));
    int new_value = lw(address, core_id);
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