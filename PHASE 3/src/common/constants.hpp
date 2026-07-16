#pragma once

// Shared compile-time constants for the Phase 3 simulator.
// Centralised here so every module (memory, pipeline, core, parser, sim)
// agrees on the same sizes instead of redefining them locally.

constexpr int MEMORY_SIZE = 4096;
constexpr int REGISTER_COUNT = 32;
constexpr int CORE_COUNT = 4;
constexpr int INSTRUCTION_MEMORY_SIZE = 1024;
