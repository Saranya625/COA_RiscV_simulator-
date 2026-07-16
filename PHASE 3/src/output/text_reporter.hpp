#pragma once
#include <vector>
#include "../core/core.hpp"

// Prints the human-readable (non-JSON) results: per-core registers,
// cycle/stall/IPC stats, scratchpad contents, and the final memory dump.
void printTextResults(std::vector<Core> &cores);
