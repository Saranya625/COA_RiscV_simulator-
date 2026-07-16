# Phase 3 simulator — module layout

The simulator used to be three large files chained together with
`#include "core.cpp"` / `#include "memory.cpp"` and a pile of global state.
It has been split into focused modules, each with a `.hpp` (declarations)
and a `.cpp` (implementation), compiled as separate translation units and
linked together. Behavior is unchanged — this was a pure structural
refactor.

```
common/       Shared compile-time constants (MEMORY_SIZE, CORE_COUNT, ...)
config/       Specifications struct + Specifications.txt parser
memory/       The whole memory hierarchy:
                memory_config        shared cache config + address helpers
                cache_replacement    templated LRU/MRU replacement policy
                main_memory          raw byte-addressable RAM + heap allocator
                data_cache           L1D/L2 data cache (lw/sw)
                instruction_cache    L1I/L2I instruction cache + instruction store
                scratchpad           per-core SP_memory (lw_spm/sw_spm)
                memory_system        initializeSystem()/printMemory() glue
                memory.hpp           umbrella header for the whole subsystem
pipeline/     PipelineStage struct + Pipeline (stage shifting/stalling)
core/         Core class: decode/execute/memory/writeback stages
parser/       parseAssembly() + label_map
sim/          fetchInstruction()/executePipeline() — the run loop, with an
                optional interactive `--step` mode (see below)
output/       JSON reporter (used by the UI) and human-readable text reporter
```

`../main.cpp` just parses CLI flags, wires these pieces together, and calls
`executePipeline()` + the appropriate reporter.

## Notable cleanups made during the split
- The instruction-cache and data-cache replacement policy functions
  (`getLRUWay`, `updateLRU`, `getMRUWay`, `updateMRU`, `get_replacement_way`,
  `updateReplacement`) used to exist twice — once for `CacheLine` and once
  more (with an `_I` suffix) for `ICacheLine`. They're now a single templated
  implementation in `memory/cache_replacement.hpp`.
- Dead/unused globals (`registers[REGISTER_COUNT]`, `memory_map`, `stalling`,
  `sync_complete`) that were left over in the old `memory.cpp`/`simulator.cpp`
  were dropped; they had zero references anywhere in the simulator.
- Everything else (pipeline hazard/forwarding logic, cache/memory timing,
  CLI flags, JSON schema) is byte-for-byte the same logic as before, just
  relocated into the module it belongs to.

## Building
From the `PHASE 3` folder, run `./build.ps1` (Windows) or `./build.sh`
(Linux/macOS/WSL) — see the scripts for the exact `g++` invocation. The
pre-refactor single-file version still exists, untouched, in `../legacy/`.

## Step-by-step mode
`executePipeline(cores, step_mode, trace_out)` in `sim/simulator_engine.cpp`
takes two optional extras, both built on the same per-cycle snapshot helper
(`buildCycleSnapshot`, defined in `trace.hpp`'s `CycleSnapshot`/
`CoreCycleSnapshot`/`StageSnapshot` types) so the two modes never disagree
about what "this cycle's state" means:

- **`--step`/`-s` (interactive CLI)** — `step_mode = true`. The run loop
  pauses after every clock cycle and prints a diff-style report (pipeline
  stage contents per core, changed registers, changed memory words) before
  waiting on stdin for `Enter` (step once more), `r` (run to completion) or
  `q` (stop early).
- **`--trace` (Web UI)** — requires `--json`. Instead of pausing, every
  cycle's snapshot is appended to a `vector<CycleSnapshot>` which
  `json_reporter.cpp` serializes as a `"trace"` array in the JSON output.
  The UI backend forwards this flag when the frontend's request sets
  `include_trace: true`, and the React "Step-by-step" tab scrubs through
  the resulting array — no live process/WebSocket needed since the whole
  run is captured up front.

Both modes are thin wrappers around the existing cycle loop — no changes to
pipeline/hazard/cache logic were needed, since `Core`/`Pipeline` state is
fully readable between cycles.
