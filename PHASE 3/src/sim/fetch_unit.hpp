#pragma once
#include <vector>
#include "../core/core.hpp"

bool check_sync_complete(std::vector<Core>& cores, int traget_address);
bool parallel_sync(std::vector<Core>& cores);
void fetchInstruction(std::vector<Core>& cores);
