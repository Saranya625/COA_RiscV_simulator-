#include "simulator_engine.hpp"
#include "fetch_unit.hpp"
#include "../common/constants.hpp"
#include "../memory/memory.hpp"
#include "../pipeline/pipeline_stage.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace {

StageSnapshot toStageSnapshot(const PipelineStage &stage) {
    StageSnapshot s;
    s.valid = stage.valid_instruction;
    if (s.valid) {
        s.instruction = stage.instruction;
        s.args = stage.args;
    }
    return s;
}

// Captures pipeline stage contents + full register file for every core,
// and records (into prev_mem, in place) any memory words that changed
// since the last time this was called. Shared by both the interactive
// `--step` printer and the `--trace` JSON collector so they never drift
// apart in what they consider "this cycle's state".
CycleSnapshot buildCycleSnapshot(vector<Core> &cores, int cycle, vector<int> &prev_mem) {
    CycleSnapshot snap;
    snap.cycle = cycle;
    snap.cores.reserve(cores.size());
    for (Core &core : cores) {
        CoreCycleSnapshot c;
        c.core_id = core.core_id;
        c.pc = core.pc;
        c.if_id  = toStageSnapshot(core.pipeline.IF_ID);
        c.id_ex  = toStageSnapshot(core.pipeline.ID_EX);
        c.ex_mem = toStageSnapshot(core.pipeline.EX_MEM);
        c.mem_wb = toStageSnapshot(core.pipeline.MEM_WB);
        c.wb     = toStageSnapshot(core.pipeline.WB_Return);
        c.registers.assign(core.registers, core.registers + REGISTER_COUNT);
        snap.cores.push_back(std::move(c));
    }
    for (int addr = 0; addr < MEMORY_SIZE; addr += 4) {
        int value = readMemoryWord(addr);
        int idx = addr / 4;
        if (value != prev_mem[idx]) {
            snap.memory_changes.push_back({addr, value});
            prev_mem[idx] = value;
        }
    }
    return snap;
}

void printStage(const char *label, const StageSnapshot &s) {
    cout << "  " << label << ": ";
    if (!s.valid) {
        cout << "-";
    } else {
        cout << s.instruction;
        for (const auto &a : s.args) cout << " " << a;
    }
    cout << "\n";
}

// Prints a snapshot, diffing its registers against the register values
// captured just before this cycle ran (regs_before) so only actual changes
// are called out.
void printSnapshot(const CycleSnapshot &snap, const vector<vector<int>> &regs_before) {
    cout << "\n================ Cycle " << snap.cycle << " ================\n";
    for (size_t i = 0; i < snap.cores.size(); i++) {
        const auto &c = snap.cores[i];
        cout << "Core " << c.core_id << " (PC=" << c.pc << ")\n";
        printStage("IF_ID ", c.if_id);
        printStage("ID_EX ", c.id_ex);
        printStage("EX_MEM", c.ex_mem);
        printStage("MEM_WB", c.mem_wb);
        printStage("WB    ", c.wb);

        bool any_reg_change = false;
        for (size_t r = 0; r < c.registers.size(); r++) {
            if (c.registers[r] != regs_before[i][r]) {
                cout << (any_reg_change ? ", " : "  Registers changed: ");
                cout << "x" << r << ": " << regs_before[i][r] << " -> " << c.registers[r];
                any_reg_change = true;
            }
        }
        if (any_reg_change) cout << "\n";
    }

    if (!snap.memory_changes.empty()) {
        cout << "Memory changed: ";
        for (size_t i = 0; i < snap.memory_changes.size(); i++) {
            if (i) cout << ", ";
            cout << "[0x" << hex << snap.memory_changes[i].address << dec << "]: "
                 << snap.memory_changes[i].value;
        }
        cout << "\n";
    }
}

} // namespace

void executePipeline(vector<Core> &cores, bool step_mode, vector<CycleSnapshot> *trace_out) {
    bool running = true;
    int cycle = 0;
    bool interactive = step_mode;

    vector<vector<int>> prev_regs(cores.size(), vector<int>(REGISTER_COUNT, 0));
    for (size_t i = 0; i < cores.size(); i++)
        for (int r = 0; r < REGISTER_COUNT; r++) prev_regs[i][r] = cores[i].registers[r];
    vector<int> prev_mem(MEMORY_SIZE / 4, 0);

    if (interactive) {
        cout << "Step mode: press Enter to advance one cycle, 'r' + Enter to run\n"
                "to completion, or 'q' + Enter to stop early.\n";
    }

    while (running) {
        for (Core &core : cores) {
            core.writeBack();
            core.memoryStage();
            core.executeStage();
            core.decodeInstruction();
        }
        fetchInstruction(cores);
        for (Core &core : cores) {
            core.pipeline.shiftStages();
        }
        cycle++;

        for (Core &core : cores) {
            if (!core.no_instructions) {
                core.clock_cycles++;
            } else if (core.no_instructions && core.barrier_sync) {
                core.clock_cycles++;
                core.pipeline.stalls++;
            }
        }

        running = false;
        for (Core &core : cores) {
            if (core.pipeline.IF_ID.valid_instruction || core.pipeline.ID_EX.valid_instruction ||
                core.pipeline.EX_MEM.valid_instruction || core.pipeline.MEM_WB.valid_instruction ||
                core.pipeline.WB_Return.valid_instruction) {
                running = true;
                break;
            }
        }

        if (interactive || trace_out) {
            vector<vector<int>> regs_before = prev_regs;
            CycleSnapshot snap = buildCycleSnapshot(cores, cycle, prev_mem);
            for (size_t i = 0; i < cores.size(); i++) prev_regs[i] = snap.cores[i].registers;

            if (trace_out) trace_out->push_back(snap);

            if (interactive) {
                printSnapshot(snap, regs_before);
                cout << "[Enter=step, r=run, q=quit] > ";
                string line;
                if (!getline(cin, line)) {
                    // No more input (e.g. piped stdin ran dry): keep running silently.
                    interactive = false;
                    continue;
                }
                if (line == "q" || line == "Q") {
                    running = false;
                } else if (line == "r" || line == "R") {
                    interactive = false;
                }
            }
        }
    }
}
