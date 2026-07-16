#pragma once
#include <vector>
#include "../core/core.hpp"
#include "../config/specifications.hpp"
#include "../sim/trace.hpp"

// Emits the full simulation result (config, per-core stats/registers,
// scratchpad, main memory, and cache hit/miss stats) as JSON on stdout.
// This is what the UI backend (ui/backend/main.py) parses via --json.
//
// When `trace` is non-null (populated via `--trace`), an additional
// "trace" array is included with one entry per clock cycle, letting the
// Web UI step through the run cycle-by-cycle after the fact.
void printJsonResults(const std::vector<Core> &cores, const Specifications &specs,
                       const std::vector<CycleSnapshot> *trace = nullptr);
