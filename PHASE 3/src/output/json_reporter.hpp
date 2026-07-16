#pragma once
#include <vector>
#include "../core/core.hpp"
#include "../config/specifications.hpp"

// Emits the full simulation result (config, per-core stats/registers,
// scratchpad, main memory, and cache hit/miss stats) as JSON on stdout.
// This is what the UI backend (ui/backend/main.py) parses via --json.
void printJsonResults(const std::vector<Core> &cores, const Specifications &specs);
