# Builds the modular Phase 3 simulator (main.cpp + src/**/*.cpp) into simulator.exe.
# Usage (from the "PHASE 3" folder):   .\build.ps1
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$sources = @((Join-Path $root "main.cpp"))
$sources += Get-ChildItem -Path (Join-Path $root "src") -Filter "*.cpp" -Recurse | ForEach-Object { $_.FullName }

$output = Join-Path $root "simulator.exe"

Write-Host "Compiling $($sources.Count) source files -> $output"
& g++ -std=c++17 -O2 -Wall -o $output @sources

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build succeeded: $output"
} else {
    Write-Host "Build failed (exit code $LASTEXITCODE)"
    exit $LASTEXITCODE
}
