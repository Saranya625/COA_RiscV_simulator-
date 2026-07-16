#pragma once
#include <string>
#include <vector>

// Per-cycle execution trace, recorded by executePipeline() when a caller
// passes a non-null trace_out vector (used by `--trace` to let the Web UI
// step through the simulation cycle-by-cycle instead of only seeing the
// final result).

struct StageSnapshot {
    bool valid = false;
    std::string instruction;
    std::vector<std::string> args;
};

struct CoreCycleSnapshot {
    int core_id = 0;
    int pc = 0;
    StageSnapshot if_id, id_ex, ex_mem, mem_wb, wb;
    std::vector<int> registers;
};

struct MemoryChange {
    int address;
    int value;
};

struct CycleSnapshot {
    int cycle = 0;
    std::vector<CoreCycleSnapshot> cores;
    std::vector<MemoryChange> memory_changes;
};
