#!/usr/bin/env bash
# Builds the modular Phase 3 simulator (main.cpp + src/**/*.cpp) into ./simulator.
# Usage (from the "PHASE 3" folder):   ./build.sh
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCES=("$ROOT_DIR/main.cpp")
while IFS= read -r -d '' f; do
    SOURCES+=("$f")
done < <(find "$ROOT_DIR/src" -name '*.cpp' -print0)

OUTPUT="$ROOT_DIR/simulator"

echo "Compiling ${#SOURCES[@]} source files -> $OUTPUT"
g++ -std=c++17 -O2 -Wall -o "$OUTPUT" "${SOURCES[@]}"
echo "Build succeeded: $OUTPUT"
