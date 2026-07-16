#pragma once
// Convenience umbrella header: everything the rest of the simulator needs
// from the memory subsystem (data cache, instruction cache, scratchpad,
// main memory, and the top-level init/print helpers). Other modules should
// generally include just this file instead of the individual pieces.

#include "memory_config.hpp"
#include "main_memory.hpp"
#include "data_cache.hpp"
#include "instruction_cache.hpp"
#include "scratchpad.hpp"
#include "memory_system.hpp"
