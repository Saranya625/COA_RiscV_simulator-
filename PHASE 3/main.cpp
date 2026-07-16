// Thin CLI entry point for the Phase 3 RISC-V simulator.
//
// All the actual logic lives under src/ (memory hierarchy, pipeline,
// core, parser, sim engine, and output reporters). This file only wires
// those pieces together: parse CLI flags -> load specs+program -> run ->
// report results.
#include <iostream>
#include <string>
#include <vector>

#include "src/common/constants.hpp"
#include "src/config/specifications.hpp"
#include "src/memory/memory.hpp"
#include "src/parser/assembly_parser.hpp"
#include "src/core/core.hpp"
#include "src/sim/simulator_engine.hpp"
#include "src/output/json_reporter.hpp"
#include "src/output/text_reporter.hpp"

int main(int argc, char *argv[]) {
    std::vector<Core> cores;
    bool json_mode = false;
    bool step_mode = false;
    bool trace_mode = false;
    std::string asm_file = "Sync.asm";
    std::string specs_file = "Specifications.txt";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--json") {
            json_mode = true;
        } else if (arg == "--step" || arg == "-s") {
            step_mode = true;
        } else if (arg == "--trace") {
            trace_mode = true;
        } else if (arg == "--specs" && i + 1 < argc) {
            specs_file = argv[++i];
        } else if (arg == "--asm" && i + 1 < argc) {
            asm_file = argv[++i];
        } else if (arg[0] != '-') {
            asm_file = arg;
        }
    }

    if (json_mode && step_mode) {
        std::cerr << "Warning: --step is ignored together with --json (JSON output must stay "
                     "machine-readable); running to completion instead.\n";
        step_mode = false;
    }

    if (trace_mode && !json_mode) {
        std::cerr << "Warning: --trace only applies together with --json; ignoring.\n";
        trace_mode = false;
    }

    if (trace_mode && step_mode) {
        std::cerr << "Warning: --trace and --step cannot be combined; ignoring --step.\n";
        step_mode = false;
    }

    Specifications specs = parseSpecifications(specs_file);
    SP_memory spm(specs.L1_cache_size, specs.L1_latency, specs.L1_associativity);

    initializeSystem(specs.line_size, specs.L1_cache_size, specs.L2_cache_size,
                      specs.L1_associativity, specs.L2_associativity,
                      specs.L1_latency, specs.L2_latency,
                      specs.replacement_policy, specs.main_memory_latency);

    bool forwarding_option = specs.data_forwarding;
    auto latencies = specs.latencies;

    if (!json_mode) {
        std::cout << "Data Forwarding: " << (forwarding_option ? "Enabled" : "Disabled") << std::endl;
        std::cout << "Latencies: " << std::endl;
        for (const auto &entry : latencies) {
            std::cout << entry.first << " -> " << entry.second << " cycles\n";
        }
    }

    parseAssembly(asm_file);
    for (int i = 0; i < CORE_COUNT; i++) {
        cores.emplace_back(i, label_map, forwarding_option, latencies, spm, specs.main_memory_latency);
    }
    std::vector<CycleSnapshot> trace;
    executePipeline(cores, step_mode, trace_mode ? &trace : nullptr);

    if (json_mode) {
        printJsonResults(cores, specs, trace_mode ? &trace : nullptr);
        return 0;
    }

    printTextResults(cores);
    return 0;
}
