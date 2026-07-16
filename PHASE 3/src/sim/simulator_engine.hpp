#pragma once
#include <vector>
#include "../core/core.hpp"
#include "trace.hpp"

// Runs the multi-core pipeline until every core has drained (no valid
// instruction left in any stage), one clock cycle at a time.
//
// step_mode: pauses after every cycle, prints what changed (pipeline stage
// contents, register writes, memory writes) and waits for the user to
// press Enter before continuing. At the prompt the user can type `r` to
// run the rest of the program without further pauses, or `q` to stop the
// simulation early. Intended for the interactive CLI (`--step`).
//
// trace_out: when non-null, a CycleSnapshot is appended to it after every
// cycle (pipeline stage contents + full register file per core, plus any
// memory words that changed that cycle). Intended for `--trace`, which lets
// the Web UI step through the run after the fact without needing a live
// process.
void executePipeline(std::vector<Core>& cores, bool step_mode = false,
                      std::vector<CycleSnapshot>* trace_out = nullptr);
