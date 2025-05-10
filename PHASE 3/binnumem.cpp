#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <deque>
#include "specifications.txt" // Assumes you have a header defining Specifications

using namespace std;

#define MEMORY_SIZE 4096
#define REGISTER_COUNT 32

uint8_t memory_main[MEMORY_SIZE] = {0};
int registers[REGISTER_COUNT] = {0};
std::unordered_map<uint32_t, uint32_t> memory_map;
uint32_t heap_pointers[4] = {0, 1024, 2048, 3072};

int total_stalls = 0;
int total_accesses = 0;
int cache_misses = 0;

Cache *L1D = nullptr;
Cache *L1I = nullptr;
Cache *L2 = nullptr;

class SP_memory
{
public:
    int size_sp;
    int latency_sp;
    int associativity_sp;
    std::vector<uint8_t> sp_memory;

    SP_memory(int size, int latency, int associativity)
        : size_sp(size), latency_sp(latency), associativity_sp(associativity), sp_memory(size, 0) {}

    int lw_spm(int address)
    {
        int value;
        std::memcpy(&value, &sp_memory[address], sizeof(int));
        return value;
    }

    void sw_spm(int address, int value)
    {
        std::memcpy(&sp_memory[address], &value, sizeof(int));
    }

    void sp_printMemory()
    {
        std::cout << "\n=== ScratchPad Memory State ===\n";
        for (int addr = 0; addr < size_sp; addr += 4)
        {
            int value = sp_memory[addr];
            std::cout << "0x" << std::hex << addr << ": " << std::dec << value << "\t";
            if ((addr / 4 + 1) % 8 == 0)
                std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

struct CacheBlock
{
    bool valid;
    uint32_t tag;
    std::vector<uint8_t> data;

    CacheBlock(int block_size) : valid(false), tag(0), data(block_size, 0) {}
};

class Cache
{
public:
    int size, block_size, associativity, latency, sets;
    string policy;
    vector<vector<CacheBlock>> cache;
    vector<deque<int>> lru_queue;

    Cache(int sz, int blk_sz, int assoc, int lat, string repl)
        : size(sz), block_size(blk_sz), associativity(assoc), latency(lat), policy(repl)
    {
        sets = size / (block_size * associativity);
        cache.resize(sets, vector<CacheBlock>(associativity, CacheBlock(block_size)));
        lru_queue.resize(sets);
        for (int i = 0; i < sets; i++)
            for (int j = 0; j < associativity; j++)
                lru_queue[i].push_back(j);
    }

    bool access(uint32_t address, bool is_write, uint8_t *buffer, bool &hit)
    {
        total_accesses++;
        uint32_t index = (address / block_size) % sets;
        uint32_t tag = address / (block_size * sets);
        uint32_t offset = address % block_size;

        for (int i = 0; i < associativity; ++i)
        {
            if (cache[index][i].valid && cache[index][i].tag == tag)
            {
                if (is_write)
                    std::memcpy(&cache[index][i].data[offset], buffer, 4);
                else
                    std::memcpy(buffer, &cache[index][i].data[offset], 4);

                if (policy == "LRU")
                {
                    lru_queue[index].erase(std::find(lru_queue[index].begin(), lru_queue[index].end(), i));
                    lru_queue[index].push_back(i);
                }

                hit = true;
                total_stalls += latency;
                return true;
            }
        }

        cache_misses++;
        hit = false;
        int replace_way = (policy == "LRU") ? lru_queue[index].front() : rand() % associativity;
        if (policy == "LRU")
        {
            lru_queue[index].pop_front();
            lru_queue[index].push_back(replace_way);
        }

        CacheBlock &block = cache[index][replace_way];
        block.valid = true;
        block.tag = tag;

        uint32_t block_addr = address - offset;
        for (int i = 0; i < block_size; ++i)
        {
            block.data[i] = memory_main[block_addr + i];
        }

        if (is_write)
        {
            std::memcpy(&block.data[offset], buffer, 4);
            std::memcpy(&memory_main[address], buffer, 4); // write-through
        }
        else
        {
            std::memcpy(buffer, &block.data[offset], 4);
        }

        total_stalls += latency + 50;
        return false;
    }
};

// Initializes global cache objects from specs
void initCaches(const specifications &specs)
{
    L1D = new Cache(specs.L1_cache_size, specs.block_size, specs.L1_associativity, specs.L1_latency, specs.replacement_policy);
    L1I = new Cache(specs.L1_cache_size, specs.block_size, specs.L1_associativity, specs.L1_latency, specs.replacement_policy);
    L2 = new Cache(specs.L2_cache_size, specs.block_size, specs.L2_associativity, specs.L2_latency, specs.replacement_policy);
}

uint32_t allocate_base_adress(int core_id)
{
    if (core_id < 0 || core_id >= 4)
    {
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }
    return heap_pointers[core_id];
}

uint32_t allocate_memory(size_t num_elements, int core_id)
{
    if (core_id < 0 || core_id >= 4)
    {
        std::cerr << "Invalid core ID: " << core_id << std::endl;
        exit(1);
    }

    uint32_t base_address = allocate_base_adress(core_id);
    uint32_t &heap_pointer = heap_pointers[core_id];

    if (heap_pointer + (num_elements * 4) >= base_address + 1024)
    {
        std::cerr << "Error: Core " << core_id << " exceeded its 1KB memory limit!\n";
        exit(1);
    }

    uint32_t allocated_address = heap_pointer;
    heap_pointer += num_elements * 4;
    return allocated_address;
}

int lw(int address)
{
    if (address < 0 || address > MEMORY_SIZE - sizeof(int))
    {
        throw std::out_of_range("Memory access out of bounds");
    }
    int value;
    std::memcpy(&value, &memory_main[address], sizeof(int));
    return value;
}

int lw(int address, int core_id)
{
    int base_address = core_id * 1024;
    if (address < base_address || address >= MEMORY_SIZE)
    {
        std::cerr << "Core " << core_id << " accessed memory out of range!\n";
        exit(1);
    }
    int value;
    std::memcpy(&value, &memory_main[address], sizeof(int));
    return value;
}

void sw(int address, int value, int core_id)
{
    int base_address = core_id * 1024;
    if (address < 0 || address >= MEMORY_SIZE)
    {
        std::cerr << "Core " << core_id << " tried to write out of range!\n";
        exit(1);
    }

    std::memcpy(&memory_main[address], &value, sizeof(int));
}

int lw1(int address, bool is_instruction = false)
{
    int value = 0;
    bool hit;

    if (is_instruction)
    {
        L1I->access(address, false, reinterpret_cast<uint8_t *>(&value), hit);
    }
    else
    {
        if (!L1D->access(address, false, reinterpret_cast<uint8_t *>(&value), hit))
        {
            L2->access(address, false, reinterpret_cast<uint8_t *>(&value), hit);
        }
    }

    return value;
}

void sw1(int address, int value)
{
    bool hit;

    if (!L1D->access(address, true, reinterpret_cast<uint8_t *>(&value), hit))
    {
        L2->access(address, true, reinterpret_cast<uint8_t *>(&value), hit);
    }
}

void printMemory()
{
    std::cout << "\n=== Memory State ===\n";
    for (int addr = 0; addr < MEMORY_SIZE; addr += 4)
    {
        int value = lw(addr);
        std::cout << "0x" << std::hex << addr << ": " << std::dec << value << "\t";
        if ((addr / 4 + 1) % 8 == 0)
            std::cout << std::endl;
    }
    std::cout << std::endl;
}

void print_stats(int total_instructions)
{
    std::cout << "\n=== Cache Statistics ===\n";
    std::cout << "Total Accesses: " << total_accesses << "\n";
    std::cout << "Cache Misses: " << cache_misses << "\n";
    std::cout << "Miss Rate: " << (float(cache_misses) / total_accesses) * 100 << "%\n";
    std::cout << "Total Stalls: " << total_stalls << "\n";
    std::cout << "IPC: " << float(total_instructions) / (total_instructions + total_stalls) << "\n";
}
