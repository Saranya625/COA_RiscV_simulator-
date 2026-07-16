#pragma once
#include <cstdint>
#include <cstddef>
#include "../common/constants.hpp"

extern uint8_t memory_main[MEMORY_SIZE];
extern uint32_t heap_pointers[4];

uint32_t allocate_base_adress(int core_id);
uint32_t allocate_memory(size_t num_elements, int core_id);
void sw1(int address, int value, int core_id);
int readMemoryWord(int address);
