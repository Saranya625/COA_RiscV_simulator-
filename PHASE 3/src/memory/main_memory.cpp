#include "main_memory.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>

uint8_t memory_main[MEMORY_SIZE] = {0};
uint32_t heap_pointers[4] = {0, 1024, 2048, 3072};

uint32_t allocate_base_adress(int core_id) {
    if (core_id < 0 || core_id >= 4) {
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }
    return heap_pointers[core_id];
}

uint32_t allocate_memory(size_t num_elements, int core_id) {
    if (core_id < 0 || core_id >= 4) {
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }

    uint32_t base_address = allocate_base_adress(core_id);
    uint32_t &heap_pointer = heap_pointers[core_id];

    if (heap_pointer + (num_elements * 4) >= base_address + 1024) {
        std::cerr << "Error: Core " << core_id << " exceeded its 1KB memory limit!\n";
        exit(1);
    }

    uint32_t allocated_address = heap_pointer;
    heap_pointer += num_elements * 4;
    return allocated_address;
}

void sw1(int address, int value, int core_id) {
    if (address < 0 || address >= MEMORY_SIZE) {
        std::cerr << "Error: Core " << core_id << " tried to access memory out of its range!\n";
        exit(1);
    }
    std::memcpy(&memory_main[address], &value, sizeof(int));
}

int readMemoryWord(int address) {
    int value = 0;
    if (address >= 0 && address + 3 < MEMORY_SIZE) {
        std::memcpy(&value, &memory_main[address], sizeof(int));
    }
    return value;
}
