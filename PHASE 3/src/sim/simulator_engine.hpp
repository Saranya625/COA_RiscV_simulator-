#pragma once
#include <vector>
#include "../core/core.hpp"

// Runs the multi-core pipeline until every core has drained (no valid
// instruction left in any stage), one clock cycle at a time.
void executePipeline(std::vector<Core>& cores);
